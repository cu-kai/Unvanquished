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
#include "common/cm/cm_public.h"
#include "sgame/sg_local.h"
#include "sgame/sg_bot_util.h"
#include "DetourDebugDraw.h"
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "DebugDraw.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include "sgame/botlib/bot_navdraw.h"
#include "shared/bot_nav_shared.h"

extern int numNavData;
extern NavData_t BotNavData[];

namespace Lua {

namespace {

static void rebuildTiles( NavData_t *nav, rVec &start, rVec &end )
{
	rVec boxMins, boxMaxs;
	for ( int i = 0; i < 3; i++ )
	{
		std::tie( boxMins[ i ], boxMaxs[ i ] ) = std::minmax( start[ i ], end[ i ] );
	}

	boxMins[ 1 ] -= 10;
	boxMaxs[ 1 ] += 10;

	// rebuild affected tiles
	dtCompressedTileRef refs[ 32 ];
	int tc = 0;
	nav->cache->queryTiles( boxMins, boxMaxs, refs, &tc, 32 );

	for ( int k = 0; k < tc; k++ )
	{
		nav->cache->buildNavMeshTile( refs[ k ], nav->mesh );
	}
}

static NavData_t *findNav( const char *navmeshName )
{
	for ( int i = 0; i < numNavData; i++ )
	{
		if ( !Q_stricmp( BG_Class( BotNavData[ i ].species )->name, navmeshName ) )
		{
			return &BotNavData[ i ];
		}
	}
	return nullptr;
}

int newNavCon( lua_State *L )
{
	const char *navmeshName = luaL_checkstring( L, 1 );
	const char *dir = luaL_checkstring( L, 2 );
	float radius = luaL_checknumber( L, 3 );
	float startx = luaL_checknumber( L, 4 );
	float starty = luaL_checknumber( L, 5 );
	float startz = luaL_checknumber( L, 6 );
	float endx = luaL_checknumber( L, 7 );
	float endy = luaL_checknumber( L, 8 );
	float endz = luaL_checknumber( L, 9 );

	OffMeshConnection pc;
	if ( Str::IsIEqual( dir, "oneway" ) )
	{
		pc.dir = 0;
	}
	else if ( Str::IsIEqual( dir, "twoway" ) )
	{
		pc.dir = 1;
	}
	else
	{
		return luaL_error( L, "addNavCon: argument #3 must be either 'oneway' or 'twoway', but is: '%s'", dir );
	}

	dtQueryFilter filter;
	pc.area = DT_TILECACHE_WALKABLE_AREA;
	pc.flag = POLYFLAGS_WALK;
	pc.userid = 0;
	pc.radius = radius;
	G_BotNavInit( 0 );
	NavData_t *nav = findNav( navmeshName );
	if ( nav == nullptr )
	{
		return luaL_error( L, "addNavCon: not a navmesh name: %s", navmeshName );
	}
	pc.start = rVec( startx, startz, starty );
	pc.end = rVec( endx, endz, endy );
	nav->process.con.addConnection( pc );
	rebuildTiles( nav, pc.start, pc.end );
	lua_pushinteger( L, nav->process.con.offMeshConCount - 1 );
	return 1;
}

int delNavCon( lua_State *L )
{
	const char *navmeshName = luaL_checkstring( L, 1 );
	int index = luaL_checkinteger( L, 2 );
	G_BotNavInit( 0 );
	NavData_t *nav = findNav( navmeshName );
	if ( nav == nullptr )
	{
		return luaL_error( L, "delNavCon: not a navmesh name: %s", navmeshName );
	}
	if ( index < 0 || index >= nav->process.con.offMeshConCount )
	{
		return luaL_error( L, "delNavCon: no such navcon: %d", index );
	}
	int n = index * 6;
	rVec start = rVec::Load( &nav->process.con.verts[ n ] );
	rVec end = rVec::Load( &nav->process.con.verts[ n + 3 ] );
	nav->process.con.delConnection( index );
	rebuildTiles( nav, start, end );
	return 0;
}

const luaL_Reg botsReg[] = {
	{ "newNavCon", newNavCon },
	{ "delNavCon", delNavCon },
	{ NULL, NULL }
};

}  // namespace

void RegisterBots( lua_State* L )
{
	luaL_newlib( L, botsReg );
	lua_setglobal( L, "Bots" );
}

}  // namespace Lua
