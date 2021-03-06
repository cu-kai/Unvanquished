/*
===========================================================================
Copyright (C) 2008-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/* vertexLighting_DBS_world_vp.glsl */

uniform mat4		u_DiffuseTextureMatrix;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_SpecularTextureMatrix;
uniform mat4		u_GlowTextureMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;
uniform vec3		u_ViewOrigin;

varying vec4		var_TexDiffuseGlow;
varying vec4		var_Color;

#if defined(USE_NORMAL_MAPPING)
varying vec4		var_TexNormalSpecular;
varying vec3		var_ViewDir;
varying vec3            var_Position;
#else
varying vec3		var_Normal;
#endif

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
	vec4 position;
	localBasis LB;
	vec2 texCoord, lmCoord;
	vec4 color;

	VertexFetch( position, LB, color, texCoord, lmCoord );

	color = color * u_ColorModulate + u_Color;

	DeformVertex( position,
		      LB.normal,
		      texCoord,
		      color,
		      u_Time);

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform diffusemap texcoords
	var_TexDiffuseGlow.st = (u_DiffuseTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	// assign color
	var_Color = color;
	
#if defined(USE_NORMAL_MAPPING)
	// transform normalmap texcoords
	var_TexNormalSpecular.st = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	// transform specularmap texture coords
	var_TexNormalSpecular.pq = (u_SpecularTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	// construct object-space-to-tangent-space 3x3 matrix
	mat3 objectToTangentMatrix = mat3( LB.tangent.x, LB.binormal.x, LB.normal.x,
					   LB.tangent.y, LB.binormal.y, LB.normal.y,
					   LB.tangent.z, LB.binormal.z, LB.normal.z );

	// assign vertex Position for light grid sampling
	var_Position = position.xyz;
	
	// assign vertex to view origin vector in tangent space
	var_ViewDir = objectToTangentMatrix * normalize( u_ViewOrigin - position.xyz );
#else
	var_Normal = LB.normal;
#endif

#if defined(USE_GLOW_MAPPING)
	var_TexDiffuseGlow.pq = ( u_GlowTextureMatrix * vec4(texCoord, 0.0, 1.0) ).st;
#endif
}
