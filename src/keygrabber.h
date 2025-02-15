/*
 * keygrabber.h - key grabbing header
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_KEYGRABBER_H
#define AWESOME_KEYGRABBER_H

#include <lua.h>
#include <xcb/xcb.h>

#include <stdbool.h>

int  luaA_keygrabber_stop(lua_State *);
bool keygrabber_handlekpress(lua_State *, xcb_key_press_event_t *);
void luaA_register_keygrabber(lua_State *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
