/*
 * mousegrabber.h - mouse pointer grabbing header
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

#ifndef AWESOME_MOUSEGRABBER_H
#define AWESOME_MOUSEGRABBER_H

#include <lua.h>
#include <xcb/xcb.h>

int  luaA_mousegrabber_stop(lua_State *);
void mousegrabber_handleevent(lua_State *, int, int, uint16_t);
void luaA_register_mousegrabber(lua_State *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
