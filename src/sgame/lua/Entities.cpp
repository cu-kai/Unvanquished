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

bool valid( int num )
{
	return num >= 0
		&& num < level.num_entities
		&& g_entities[ num ].inuse;
}

bool validClient( int num )
{
	return valid( num )
		&& num < MAX_CLIENTS
		&& g_entities[ num ].client;  // should be redundant but cannot hurt
}

bool validNonClient( int num )
{
	return valid( num ) && num >= MAX_CLIENTS;
}

int isClient( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	lua_pushboolean( L, validClient( num ) );
	return 1;
}

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

int numToId( lua_State *L )
{
	int entityNum = luaL_checkinteger( L, 1 );
	if ( !validNonClient( entityNum )
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

int inUse( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	lua_pushboolean( L, validNonClient( num ) || validClient( num ) );
	return 1;
}

int getter( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !valid( num ) )
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
	if ( !valid( num ) )
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

int getterNonClient( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validNonClient( num ) )
	{
		lua_pushnil( L );
	}
	else
	{
		f( L, num );
	}
	return 1;
}

int getterNonClient3( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validNonClient( num ) )
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

int getterClient( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validClient( num ) || g_entities[ num ].client->pers.team == TEAM_NONE )
	{
		lua_pushnil( L );
	}
	else
	{
		f( L, num );
	}
	return 1;
}

int getterClient2( lua_State *L, void (*f) (lua_State *L, int num ) )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validClient( num ) || g_entities[ num ].client->pers.team == TEAM_NONE )
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

int classname( lua_State *L )
{
	return getterNonClient( L, []( lua_State *L, int num ) {
		lua_pushstring( L, g_entities[ num ].classname );
	});
}

int clientNum( lua_State* L ) {
	return getterClient( L, []( lua_State* L, int num ) {
		lua_pushinteger( L, g_entities[num].client->num() );
		} );
}

int origin( lua_State *L )
{
	return getter3( L, []( lua_State *L, int num ) {
		gentity_t *e = &g_entities[ num ];
		if ( e->s.eType == entityType_t::ET_MOVER )
		{
			for ( int i = 0; i < 3; i++ )
			{
				lua_pushnumber( L, ( e->r.absmax[ i ] + e->r.absmin[ i ] ) / 2.f );
			}
		}
		else
		{
			for ( int i = 0; i < 3; i++ )
			{
				lua_pushnumber( L, e->s.origin[ i ] );
			}
		}
	});
}

int angles( lua_State *L )
{
	return getterNonClient3( L, []( lua_State *L, int num ) {
		for ( int i = 0; i < 3; i++ )
		{
			lua_pushnumber( L, g_entities[ num ].s.angles[ i ] );
		}
	});
}

int enabled( lua_State *L )
{
	return getterNonClient( L, []( lua_State *L, int num ) {
		lua_pushboolean( L, g_entities[ num ].enabled );
	});
}

int locked( lua_State *L )
{
	return getterNonClient( L, []( lua_State *L, int num ) {
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

int blasterActive( lua_State *L )
{
	return getterClient( L, []( lua_State *L, int num ) {
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
	if ( !validClient( num ) )
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

int team( lua_State *L )
{
	return getter( L, []( lua_State *L, int num ) {
		if ( num < MAX_CLIENTS )
		{
			lua_pushstring( L, BG_TeamNamePlural( g_entities[ num ].client->pers.team ) );
		}
		else if ( g_entities[ num ].s.eType == entityType_t::ET_BUILDABLE )
		{
			lua_pushstring( L, BG_TeamNamePlural( g_entities[ num ].buildableTeam ) );
		}
		else
		{
			lua_pushnil( L );
		}
	});
}

int items( lua_State *L )
{
	return getterClient( L, []( lua_State *L, int num ) {
		lua_pushinteger( L, g_entities[ num ].client->ps.stats[ STAT_ITEMS ] );
	});
}

int weapon( lua_State *L )
{
	return getterClient( L, []( lua_State *L, int num ) {
		lua_pushinteger( L, g_entities[ num ].client->ps.stats[ STAT_WEAPON ] );
	});
}

int ammo( lua_State *L )
{
	return getterClient2( L, []( lua_State *L, int num ) {
		lua_pushinteger( L, g_entities[ num ].client->ps.ammo );
		lua_pushinteger( L, g_entities[ num ].client->ps.clips );
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
	if ( validNonClient( num ) )
	{
		G_FreeEntity( &g_entities[ num ] );
	}
	return 0;
}

int parent( lua_State *L )
{
	return getterNonClient( L, []( lua_State *L, int num ) {
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

int group( lua_State *L )
{
	return getterNonClient( L, []( lua_State *L, int num ) {
		if ( g_entities[ num ].mapEntity.groupName == nullptr )
		{
			lua_pushnil( L );
		}
		else
		{
			lua_pushstring( L, g_entities[ num ].mapEntity.groupName );
		}
	});
}

int bounds( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	if ( !validNonClient( num ) )
	{
		for ( int i = 0; i < 6; i++ )
		{
			lua_pushnil( L );
		}
		return 6;
	}
	gentity_t *ent = &g_entities[ num ];
	lua_pushnumber( L, ent->r.mins[ 0 ] );
	lua_pushnumber( L, ent->r.mins[ 1 ] );
	lua_pushnumber( L, ent->r.mins[ 2 ] );
	lua_pushnumber( L, ent->r.maxs[ 0 ] );
	lua_pushnumber( L, ent->r.maxs[ 1 ] );
	lua_pushnumber( L, ent->r.maxs[ 2 ] );
	return 6;
}

int newBuildable( lua_State *L )
{
	const char *buildableName = luaL_checkstring( L, 1 );
	const buildableAttributes_t *attr = BG_BuildableByName( buildableName );
	buildable_t buildable = attr->number;
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
	level.team[ attr->team ].spentBudget += attr->buildPoints;
	return 0;
}

int newObjective( lua_State* L ) {
	int id = luaL_checknumber( L, 1 );

	glm::vec3 origin { luaL_checknumber( L, 2 ),
					   luaL_checknumber( L, 3 ),
					   luaL_checknumber( L, 4 ) };

	gentity_t* beacon = Beacon::New( origin, BCT_OBJECTIVE, id, team_t::TEAM_ALL );
	Beacon::Propagate( beacon );

	// G_SetAutomaticEntityId( beacon );
	// G_RegisterEntityId( beacon->num(), beacon->id );

	lua_pushinteger( L, beacon->num() );
	return 1;
}

int deleteObjective( lua_State* L ) {
	int entityID = luaL_checknumber( L, 1 );
	if ( !validNonClient( entityID ) ) {
		return 0;
	}

	gentity_t* entity = &g_entities[entityID];
	if ( entity->s.eType != entityType_t::ET_BEACON || entity->s.modelindex != BCT_OBJECTIVE ) {
		return 0;
	}

	Beacon::Delete( entity );

	return 0;
}

int setObjectiveString( lua_State* L ) {
	int entityID = luaL_checknumber( L, 1 );
	if ( !validNonClient( entityID ) ) {
		return 0;
	}

	gentity_t* entity = &g_entities[entityID];
	if ( entity->s.eType != entityType_t::ET_BEACON || entity->s.modelindex != BCT_OBJECTIVE ) {
		return 0;
	}

	int id = luaL_checknumber( L, 2 );
	entity->s.modelindex2 = id;

	Beacon::UpdateTags( entity );

	return 0;
}

int setCustomString( lua_State* L ) {
	int client = luaL_checknumber( L, 1 );

	int id = luaL_checknumber( L, 2 );

	const char* customString = luaL_checkstring( L, 3 );

	trap_SendServerCommand( client, Str::Format( "set_custom_string %i %s ", id, Quote( customString ) ).c_str() );

	return 0;
}

int damage( lua_State *L )
{
	int num = luaL_checknumber( L, 1 );
	int amount = luaL_checknumber( L, 2 );
	// TODO: add arguments for inflictor and means of death
	if ( !valid( num ) )
	{
		return luaL_error( L, "damage: not a valid entity: %d", num );
	}
	if ( num >= MAX_CLIENTS && g_entities[ num ].s.eType != entityType_t::ET_BUILDABLE )
	{
		return luaL_error( L, "damage: neither a client nor a buildable: %d", num );
	}
	g_entities[ num ].Damage( amount, nullptr, Util::nullopt, Util::nullopt, DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT );
	return 0;
}

const luaL_Reg entitiesReg[] = {
	{ "idToNum", idToNum },
	{ "numToId", numToId },
	{ "inUse", inUse },
	{ "isClient", isClient },
	{ "classname", classname },
	{ "clientNum", clientNum },
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
	{ "newObjective", newObjective },
	{ "deleteObjective", deleteObjective },
	{ "setObjectiveString", setObjectiveString },
	{ "setCustomString", setCustomString },
	{ "group", group },
	{ "bounds", bounds },
	{ "ammo", ammo },
	{ "weapon", weapon },
	{ "items", items },
	{ "spawnAt", spawnAt },
	{ "centerPrint", centerPrint },
	{ "blasterActive", blasterActive },
	{ "damage", damage },
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
