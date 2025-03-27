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

int idToNum( lua_State *L )
{
	const char *id = luaL_checkstring(L, 1);
	int entityNum = G_IdToEntityNum( id );
	if ( entityNum >= 0 )
	{
		lua_pushinteger( L, entityNum );
	}
	else
	{
		lua_pushnil( L );
	}
	return 1;
}

bool validEntityNum( int num )
{
	return num >= MAX_CLIENTS
		&& num < level.num_entities
		&& g_entities[ num ].inuse;
}

int numToId( lua_State *L )
{
	int entityNum = luaL_checkinteger( L, 1 );
	if ( !validEntityNum( entityNum )
	     || g_entities[ entityNum ].id == nullptr )
	{
		lua_pushnil( L );
	}
	else
	{
		lua_pushstring( L, g_entities[ entityNum ].id );
	}
	return 1;
}

int isNum( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	lua_pushboolean( L, validEntityNum( num ) );
	return 1;
}

int getter( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validEntityNum( num ) )
	{
		lua_pushnil( L );
	}
	else
	{
		f( L, num );
	}
	return 1;
}

int getter3( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validEntityNum( num ) )
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

int classname( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		lua_pushstring( L, g_entities[ num ].classname );
	});
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

int angles( lua_State *L )
{
	return getter3( L, []( lua_State *L, int num ) {
		for ( int i = 0; i < 3; i++ )
		{
			lua_pushnumber( L, g_entities[ num ].s.angles[ i ] );
		}
	});
}

int enabled( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		lua_pushboolean( L, g_entities[ num ].enabled );
	});
}

int team( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		if ( g_entities[ num ].s.eType != entityType_t::ET_BUILDABLE )
		{
			lua_pushnil( L );
		}
		else
		{
			lua_pushstring( L, BG_TeamNamePlural( g_entities[ num ].buildableTeam ) );
		}
	});
}

const luaL_Reg entitiesReg[] = {
	{ "idToNum", idToNum },
	{ "numToId", numToId },
	{ "isNum", isNum },
	{ "classname", classname },
	{ "origin", origin },
	{ "angles", angles },
	{ "enabled", enabled },
	{ "team", team },
	{ NULL, NULL }
};

}  // namespace

int EntityHandlersRegistryHandle = 0;

void RegisterEntities( lua_State* L )
{
	luaL_newlib( L, entitiesReg );
	lua_newtable( L );
	lua_setfield( L, -2, "handlers" );
	lua_setglobal( L, "Entities" );

	// put Entities.handlers in the registry
	lua_getglobal( L, "Entities" );
	lua_pushstring( L, "handlers" );
	lua_gettable( L, -2 );
	EntityHandlersRegistryHandle = luaL_ref(L, LUA_REGISTRYINDEX );
	lua_pop( L, 1 );
}

}  // namespace Lua
