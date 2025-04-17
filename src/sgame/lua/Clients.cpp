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
#include "sgame/CBSE.h"

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

int getter3( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validClientNum( num ) || g_entities[ num ].client->pers.team == TEAM_NONE )
	{
		lua_pushnil( L );
		lua_pushnil( L );
		lua_pushnil( L );
	}
	else
	{
		f( L, num );
	}
	return 3;
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

int health( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		if ( g_entities[ num ].client->pers.team == TEAM_NONE )
		{
			lua_pushnil( L );
		}
		else
		{
			lua_pushinteger( L, g_entities[ num ].entity->Get<HealthComponent>()->Health() );
		}
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

int centerPrint( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	const char *message = luaL_checkstring( L, 2 );
	if ( !validClientNum( num ) )
	{
		return luaL_error( L, "centerPrint: not a client number: %d", num );
	}
	if ( g_entities[ num ].r.svFlags & SVF_BOT )
	{
		return 0;
	}
	std::string cmd = Str::Format( "cp_tr %s", Quote( message ) );
	trap_SendServerCommand( num, cmd.c_str() );
	return 0;
}

int origin( lua_State *L )
{
	return getter3( L, []( lua_State *L, int num ) {
		for ( int i = 0; i < 3; i++ )
		{
			lua_pushnumber( L, g_entities[ num ].s.origin[ i ] );
		}
	});
}

int spawnAt( lua_State *L )
{
	const char *teamStr = luaL_checkstring( L, 1 );
	team_t team;
	if ( Str::IsIEqual( teamStr, "aliens" ) )
	{
		team = TEAM_ALIENS;
	}
	else if ( Str::IsIEqual( teamStr, "humans" ) )
	{
		team = TEAM_HUMANS;
	}
	else
	{
		return luaL_error( L, "spawnAt: not a team: %s", teamStr );
	}
	double x, y, z;
	if ( lua_isnil( L, 2 ) )
	{
		preferredSpawnLocations[ team ] = Util::nullopt;
	}
	else
	{
		x = luaL_checknumber( L, 2 );
		y = luaL_checknumber( L, 3 );
		z = luaL_checknumber( L, 4 );
		preferredSpawnLocations[ team ] = glm::vec3( x, y, z);
	}
	return 0;
}

const luaL_Reg clientsReg[] = {
	{ "isNum", isNum },
	{ "team", team },
	{ "items", items},
	{ "weapon", weapon},
	{ "ammo", ammo},
	{ "blasterActive", blasterActive},
	{ "centerPrint", centerPrint},
	{ "health", health },
	{ "origin", origin },
	{ "spawnAt", spawnAt },
	{ NULL, NULL }
};

}  // namespace

void RegisterClients( lua_State* L )
{
	luaL_newlib( L, clientsReg );
	lua_setglobal( L, "Clients" );
}

}  // namespace Lua
