/*
 * selection_acquire.c - objects for selection ownership
 *
 * Copyright © 2019 Uli Schlachter <psychon@znc.in>
 * Copyright © 2023 Abigail Teague <ateague063@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "objects/selection_acquire.h"
#include "common/lualib.h"
#include "common/object.h"
#include "globalconf.h"
#include "objects/selection_transfer.h"

#define REGISTRY_ACQUIRE_TABLE_INDEX "luna_selection_acquires"

typedef struct selection_acquire_t {
    /** The selection that is being owned. */
    xcb_atom_t      selection;
    /** Window used for owning the selection. */
    xcb_window_t    window;
    /** Timestamp used for acquiring the selection. */
    xcb_timestamp_t timestamp;
} selection_acquire_t;

static void lunaL_selection_acquire_alloc(lua_State *L) {
    selection_acquire_t *s = lua_newuserdatauv(L, sizeof(selection_acquire_t), 1);
    p_clear(s, 1);
}

static void luaA_pushatom(lua_State *L, xcb_atom_t atom) {
    lua_pushnumber(L, atom);
}

static int selection_acquire_find_by_window(lua_State *L, xcb_window_t window) {
    /* Iterate over all active selection acquire objects */
    lua_pushliteral(L, REGISTRY_ACQUIRE_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -1) == LUA_TUSERDATA) {
            selection_acquire_t *selection = lua_touserdata(L, -1);
            if (selection->window == window) {
                /* Remove table and key */
                lua_remove(L, -2);
                lua_remove(L, -2);
                return 1;
            }
        }
        /* Remove the value, leaving only the key */
        lua_pop(L, 1);
    }
    /* Remove the table */
    lua_pop(L, 1);

    return 0;
}

static void selection_release(lua_State *L, int ud) {
    selection_acquire_t *selection = luaC_checkuclass(L, ud, "SelectionAcquire");

    luna_object_emit_signal(L, ud, "release", 0);

    /* Destroy the window, this also releases the selection in X11 */
    xcb_destroy_window(globalconf.connection, selection->window);
    selection->window = XCB_NONE;

    /* Unreference the object, it's now dead */
    lua_pushliteral(L, REGISTRY_ACQUIRE_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);
    luaA_pushatom(L, selection->selection);
    lua_pushnil(L);
    lua_rawset(L, -3);

    selection->selection = XCB_NONE;

    lua_pop(L, 1);
}

void selection_handle_selectionclear(xcb_selection_clear_event_t *ev) {
    lua_State *L = globalconf_get_lua_State();

    if (selection_acquire_find_by_window(L, ev->owner) == 0) return;

    selection_release(L, -1);
    lua_pop(L, 1);
}

void selection_handle_selectionrequest(xcb_selection_request_event_t *ev) {
    lua_State *L = globalconf_get_lua_State();

    if (ev->property == XCB_NONE) /* Obsolete client */
        ev->property = ev->target;

    if (selection_acquire_find_by_window(L, ev->owner) == 0) {
        selection_transfer_reject(ev->requestor, ev->selection, ev->target, ev->time);
        return;
    }

    selection_transfer_begin(
        L, -1, ev->requestor, ev->selection, ev->target, ev->property, ev->time);

    lua_pop(L, 1);
}

static int luaA_selection_acquire_new(lua_State *L) {
    size_t                           name_length;
    const char                      *name;
    xcb_intern_atom_reply_t         *reply;
    xcb_get_selection_owner_reply_t *selection_reply;
    xcb_atom_t                       name_atom;
    selection_acquire_t             *selection;

    luaA_checktable(L, 2);
    lua_pushliteral(L, "selection");
    lua_gettable(L, 2);
    name  = luaL_checklstring(L, -1, &name_length);

    /* Get the atom identifying the selection */
    reply = xcb_intern_atom_reply(
        globalconf.connection,
        xcb_intern_atom_unchecked(globalconf.connection, false, name_length, name), NULL);
    name_atom = reply ? reply->atom : XCB_NONE;
    p_delete(&reply);

    selection            = lua_touserdata(L, 1);
    selection->selection = name_atom;
    selection->timestamp = globalconf.timestamp;
    selection->window    = xcb_generate_id(globalconf.connection);
    xcb_create_window(
        globalconf.connection, globalconf.screen->root_depth, selection->window,
        globalconf.screen->root, -1, -1, 1, 1, 0, XCB_COPY_FROM_PARENT,
        globalconf.screen->root_visual, 0, NULL);

    /* Try to acquire the selection */
    xcb_set_selection_owner(
        globalconf.connection, selection->window, name_atom, selection->timestamp);
    selection_reply = xcb_get_selection_owner_reply(
        globalconf.connection, xcb_get_selection_owner(globalconf.connection, name_atom), NULL);
    if (selection_reply == NULL || selection_reply->owner != selection->window) {
        /* Acquiring the selection failed, return nothing */
        p_delete(&selection_reply);

        xcb_destroy_window(globalconf.connection, selection->window);
        selection->window = XCB_NONE;
        return 0;
    }

    /* Everything worked, register the object in the table */
    lua_pushliteral(L, REGISTRY_ACQUIRE_TABLE_INDEX);
    lua_rawget(L, LUA_REGISTRYINDEX);

    luaA_pushatom(L, name_atom);
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1)) {
        /* There is already another selection_acquire object for this selection,
         * release it now. X11 does not send us SelectionClear events for our
         * own changes to the selection.
         */
        selection_release(L, -1);
    }

    luaA_pushatom(L, name_atom);
    lua_pushvalue(L, 1);
    lua_rawset(L, -4);
    lua_pop(L, 2);

    return 1;
}

static int luaA_selection_acquire_release(lua_State *L) {
    selection_release(L, 1);
    return 0;
}

static bool selection_acquire_checker(selection_acquire_t *selection) {
    return selection->selection != XCB_NONE;
}

static luaL_Reg selection_acquire_methods[] = {
    {"new",     luaA_selection_acquire_new    },
    {"release", luaA_selection_acquire_release},
    {NULL,      NULL                          }
};

static luaC_Class selection_acquire_class = {
    .name      = "SelectionAcquire",
    .parent    = "Object",
    .user_ctor = 1,
    .alloc     = lunaL_selection_acquire_alloc,
    .gc        = NULL,
    .methods   = selection_acquire_methods};

void luaC_register_selection_acquire(lua_State *L) {
    lua_pushlightuserdata(L, &selection_acquire_class);
    luaC_register(L, -1);

    /* Store a table in the registry that tracks active selection_acquire_t. */
    lua_pushliteral(L, REGISTRY_ACQUIRE_TABLE_INDEX);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);

    lua_pop(L, 1);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
