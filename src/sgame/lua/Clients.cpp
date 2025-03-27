/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2025 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "shared/bg_lua.h"
#include "common/Common.h"
#include "sgame/sg_local.h"
#include "sgame/sg_entities.h"

namespace Lua {

namespace {

bool validClientNum( int num )
{
	return num >= 0
		&& num < MAX_CLIENTS
		&& g_entities[ num ].client
		&& g_entities[ num ].inuse;
}

int isNum( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	lua_pushboolean( L, validClientNum( num ) );
	return 1;
}

int team( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validClientNum( num )
	     || g_entities[ num ].client->pers.team == TEAM_NONE )
	{
		lua_pushnil( L );
	}
	else
	{
		lua_pushstring( L, BG_TeamNamePlural( g_entities[ num ].client->pers.team ) );
	}
	return 1;
}

int getter( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validClientNum( num ) || g_entities[ num ].client->pers.team == TEAM_NONE )
	{
		lua_pushnil( L );
	}
	else
	{
		f( L, num );
	}
	return 1;
}

int getter2( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validClientNum( num ) || g_entities[ num ].client->pers.team == TEAM_NONE )
	{
		lua_pushnil( L );
		lua_pushnil( L );
	}
	else
	{
		f( L, num );
	}
	return 2;
}

int items( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		lua_pushinteger( L, g_entities[ num ].client->ps.stats[ STAT_ITEMS ] );
	});
}

int weapon( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		lua_pushinteger( L, g_entities[ num ].client->ps.stats[ STAT_WEAPON ] );
	});
}

int ammo( lua_State *L )
{
	return getter2( L, []( lua_State *L, int num ) {
		lua_pushinteger( L, g_entities[ num ].client->ps.ammo );
		lua_pushinteger( L, g_entities[ num ].client->ps.clips );
	});
}

int blasterActive( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		if ( g_entities[ num ].client->pers.team == TEAM_HUMANS )
		{
			lua_pushboolean( L, BG_GetPlayerWeapon( &g_entities[ num ].client->ps ) == WP_BLASTER );
		}
		else
		{
			lua_pushnil( L );
		}
	});
}

const luaL_Reg clientsReg[] = {
	{ "isNum", isNum },
	{ "team", team },
	{ "items", items},
	{ "weapon", weapon},
	{ "ammo", ammo},
	{ "blasterActive", blasterActive},
	{ NULL, NULL }
};

}  // namespace

void RegisterClients( lua_State* L )
{
	luaL_newlib( L, clientsReg );
	lua_setglobal( L, "Clients" );
}

}  // namespace Lua
