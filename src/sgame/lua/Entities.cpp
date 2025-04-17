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

int locked( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		if ( g_entities[ num ].s.eType != entityType_t::ET_MOVER )
		{
			lua_pushnil( L );
		}
		else
		{
			lua_pushboolean( L, g_entities[ num ].mapEntity.locked );
		}
	});
}

int health( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		gentity_t *ent = &g_entities[ num ];
		if ( HasComponents<HealthComponent>(*ent->entity) )
		{
			lua_pushinteger( L, ent->entity->Get<HealthComponent>()->Health() );
		}
		else if ( Str::IsIEqual( g_entities[ num ].classname, "func_destructable" ) )
		{
			lua_pushinteger( L, ent->health );
		}
		else
		{
			lua_pushnil( L );
		}
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

static void trigger_touch( gentity_t *self, gentity_t *activator )
{
	// TODO: this is currently limited to firing at most once per second
	// if two clients touch the trigger in the same second, only one of them
	// causes this function to be called
	// that is a problem if we want to change the clients' own states, like
	// increasing their health
	static int lastTouchTime[ MAX_GENTITIES ] = { 0 };
	if ( activator->num() >= MAX_CLIENTS || level.time - lastTouchTime[ self->num() ] < 1000 )
	{
		return;
	}
	lastTouchTime[ self->num() ] = level.time;
	CallLuaEntityHandler( self, "default", activator );
}

int newSensor( lua_State *L )
{
	// currently, we make a sensor entity created by this fire once every second
	// maybe we should allow to customize that interval by a 7th argument
	vec3_t mins, maxs;
	mins[ 0 ] = luaL_checknumber( L, 1 );
	mins[ 1 ] = luaL_checknumber( L, 2 );
	mins[ 2 ] = luaL_checknumber( L, 3 );
	maxs[ 0 ] = luaL_checknumber( L, 4 );
	maxs[ 1 ] = luaL_checknumber( L, 5 );
	maxs[ 2 ] = luaL_checknumber( L, 6 );

	gentity_t *newEntity = G_NewEntity( NO_CBSE );  // yes!!
	newEntity->classname = S_DOOR_SENSOR;  // for now
	VectorCopy( mins, newEntity->r.mins );
	VectorCopy( maxs, newEntity->r.maxs );
	newEntity->parent = newEntity;
	newEntity->r.contents = CONTENTS_TRIGGER;
	newEntity->touch = trigger_touch;
	trap_LinkEntity( newEntity );
	G_SetAutomaticEntityId( newEntity );
	G_RegisterEntityId( newEntity->num(), newEntity->id );

	lua_pushinteger( L, newEntity->num() );
	return 1;
}

int deleteEntity( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	if ( validEntityNum( num ) )
	{
		G_FreeEntity( &g_entities[ num ] );
	}
	return 0;
}

int parent( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		if ( g_entities[ num ].parent == nullptr )
		{
			lua_pushnil( L );
		}
		else
		{
			lua_pushinteger( L, g_entities[ num ].parent->num() );
		}
	});
}

int newBuildable( lua_State *L )
{
	const char *buildableName = luaL_checkstring( L, 1 );
	buildable_t buildable = BG_BuildableByName( buildableName )->number;
	if ( buildable <= 0 )
	{
		return luaL_error( L, "newBuildable: not a buildable: %s", buildableName );
	}
	lua_pushinteger( L, buildable );

	vec3_t origin;
	origin[ 0 ] = luaL_checknumber( L, 2 );
	origin[ 1 ] = luaL_checknumber( L, 3 );
	origin[ 2 ] = luaL_checknumber( L, 4 );

	vec3_t angles;
	angles[ 0 ] = luaL_checknumber( L, 5 );
	angles[ 1 ] = luaL_checknumber( L, 6 );
	angles[ 2 ] = luaL_checknumber( L, 7 );

	vec3_t origin2;
	origin2[ 0 ] = luaL_checknumber( L, 8 );
	origin2[ 1 ] = luaL_checknumber( L, 9 );
	origin2[ 2 ] = luaL_checknumber( L, 10 );

	vec3_t angles2;
	angles2[ 0 ] = luaL_checknumber( L, 11 );
	angles2[ 1 ] = luaL_checknumber( L, 12 );
	angles2[ 2 ] = luaL_checknumber( L, 13 );

	gentity_t *ent = G_NewEntity( NO_CBSE );
	VectorCopy( origin, ent->s.pos.trBase );
	VectorCopy( angles, ent->s.angles );
	VectorCopy( origin2, ent->s.origin2 );
	VectorCopy( angles2, ent->s.angles2 );
	G_SpawnBuildable( ent, buildable );
	return 0;
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
	{ "health", health },
	{ "locked", locked },
	{ "newSensor", newSensor },
	{ "delete", deleteEntity },
	{ "parent", parent },
	{ "newBuildable", newBuildable },
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
