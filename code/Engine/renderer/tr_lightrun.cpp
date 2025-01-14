/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "engine_precompiled.h"
#pragma hdrstop

#include "tr_local.h"

/*


Prelight models

"_prelight_<lightname>", ie "_prelight_light1"

Static surfaces available to dmap will be processed to optimized
shadow and lit surface geometry

Entity models are never prelighted.

Light entity can have a "noPrelight 1" key set to avoid the preprocessing
and carving of the world.  A light that will move should usually have this
set.

Prelight models will usually have multiple surfaces

Shadow volume surfaces will have the material "_shadowVolume"

The exact same vertexes as the ambient surfaces will be used for the
non-shadow surfaces, so there is opportunity to share


Reference their parent surfaces?
Reference their parent area?


If we don't track parts that are in different areas, there will be huge
losses when an areaportal closed door has a light poking slightly
through it.

There is potential benefit to splitting even the shadow volumes
at area boundaries, but it would involve the possibility of an
extra plane of shadow drawing at the area boundary.


interaction	lightName	numIndexes

Shadow volume surface

Surfaces in the world cannot have "no self shadow" properties, because all
the surfaces are considered together for the optimized shadow volume.  If
you want no self shadow on a static surface, you must still make it into an
entity so it isn't considered in the prelight.


r_hidePrelights
r_hideNonPrelights



each surface could include prelight indexes

generation procedure in dmap:

carve original surfaces into areas

for each light
	build shadow volume and beam tree
	cut all potentially lit surfaces into the beam tree
		move lit fragments into a new optimize group

optimize groups

build light models




*/

/*
=================================================================================

LIGHT TESTING

=================================================================================
*/


/*
====================
R_ModulateLights_f

Modifies the shaderParms on all the lights so the level
designers can easily test different color schemes
====================
*/
void R_ModulateLights_f( const idCmdArgs &args ) {
	
}



//======================================================================================


/*
===============
R_CreateEntityRefs

Creates all needed model references in portal areas,
chaining them to both the area and the entityDef.

Bumps tr.viewCount.
===============
*/
void R_CreateEntityRefs( idRenderEntityLocal *def ) {
	int			i;
	idVec3		transformed[8];
	idVec3		v;

	if ( !def->parms.hModel ) {
		def->parms.hModel = renderModelManager->DefaultModel();
	}

	// if the entity hasn't been fully specified due to expensive animation calcs
	// for md5 and particles, use the provided conservative bounds.
	if ( def->parms.callback ) {
		def->referenceBounds = def->parms.bounds;
	} else {
		def->referenceBounds = def->parms.hModel->Bounds( def );
	}

	// some models, like empty particles, may not need to be added at all
	if ( def->referenceBounds.IsCleared() ) {
		return;
	}

	if ( r_showUpdates.GetBool() && 
		( def->referenceBounds[1][0] - def->referenceBounds[0][0] > 1024 ||
		def->referenceBounds[1][1] - def->referenceBounds[0][1] > 1024 )  ) {
		common->Printf( "big entityRef: %f,%f\n", def->referenceBounds[1][0] - def->referenceBounds[0][0],
						def->referenceBounds[1][1] - def->referenceBounds[0][1] );
	}

	for (i = 0 ; i < 8 ; i++) {
		v[0] = def->referenceBounds[i&1][0];
		v[1] = def->referenceBounds[(i>>1)&1][1];
		v[2] = def->referenceBounds[(i>>2)&1][2];

		R_LocalPointToGlobal( def->modelMatrix, v, transformed[i] ); 
	}

	// bump the view count so we can tell if an
	// area already has a reference
	tr.viewCount++;
}


/*
=================================================================================

CREATE LIGHT REFS

=================================================================================
*/

/*
=====================
R_SetLightProject

All values are reletive to the origin
Assumes that right and up are not normalized
This is also called by dmap during map processing.
=====================
*/
void R_SetLightProject( idPlane lightProject[4], const idVec3 origin, const idVec3 target,
					   const idVec3 rightVector, const idVec3 upVector, const idVec3 start, const idVec3 stop ) {
	float		dist;
	float		scale;
	float		rLen, uLen;
	idVec3		normal;
	float		ofs;
	idVec3		right, up;
	idVec3		startGlobal;
	idVec4		targetGlobal;

	right = rightVector;
	rLen = right.Normalize();
	up = upVector;
	uLen = up.Normalize();
	normal = up.Cross( right );
//normal = right.Cross( up );
	normal.Normalize();

	dist = target * normal; //  - ( origin * normal );
	if ( dist < 0 ) {
		dist = -dist;
		normal = -normal;
	}

	scale = ( 0.5f * dist ) / rLen;
	right *= scale;
	scale = -( 0.5f * dist ) / uLen;
	up *= scale;

	lightProject[2] = normal;
	lightProject[2][3] = -( origin * lightProject[2].Normal() );

	lightProject[0] = right;
	lightProject[0][3] = -( origin * lightProject[0].Normal() );

	lightProject[1] = up;
	lightProject[1][3] = -( origin * lightProject[1].Normal() );

	// now offset to center
	targetGlobal.ToVec3() = target + origin;
	targetGlobal[3] = 1;
	ofs = 0.5f - ( targetGlobal * lightProject[0].ToVec4() ) / ( targetGlobal * lightProject[2].ToVec4() );
	lightProject[0].ToVec4() += ofs * lightProject[2].ToVec4();
	ofs = 0.5f - ( targetGlobal * lightProject[1].ToVec4() ) / ( targetGlobal * lightProject[2].ToVec4() );
	lightProject[1].ToVec4() += ofs * lightProject[2].ToVec4();

	// set the falloff vector
	normal = stop - start;
	dist = normal.Normalize();
	if ( dist <= 0 ) {
		dist = 1;
	}
	lightProject[3] = normal * ( 1.0f / dist );
	startGlobal = start + origin;
	lightProject[3][3] = -( startGlobal * lightProject[3].Normal() );
}

/*
===================
R_SetLightFrustum

Creates plane equations from the light projection, positive sides
face out of the light
===================
*/
void R_SetLightFrustum( const idPlane lightProject[4], idPlane frustum[6] ) {
	int		i;

	// we want the planes of s=0, s=q, t=0, and t=q
	frustum[0] = lightProject[0];
	frustum[1] = lightProject[1];
	frustum[2] = lightProject[2] - lightProject[0];
	frustum[3] = lightProject[2] - lightProject[1];

	// we want the planes of s=0 and s=1 for front and rear clipping planes
	frustum[4] = lightProject[3];

	frustum[5] = lightProject[3];
	frustum[5][3] -= 1.0f;
	frustum[5] = -frustum[5];

	for ( i = 0 ; i < 6 ; i++ ) {
		float	l;

		frustum[i] = -frustum[i];
		l = frustum[i].Normalize();
		frustum[i][3] /= l;
	}
}

/*
====================
R_FreeLightDefFrustum
====================
*/
void R_FreeLightDefFrustum( idRenderLightLocal *ldef ) {
	int i;

	// free the frustum tris
	if ( ldef->frustumTris ) {
		R_FreeStaticTriSurf( ldef->frustumTris );
		ldef->frustumTris = NULL;
	}
	// free frustum windings
	for ( i = 0; i < 6; i++ ) {
		if ( ldef->frustumWindings[i] ) {
			delete ldef->frustumWindings[i];
			ldef->frustumWindings[i] = NULL;
		}
	}
}


/*
========================
R_ComputePointLightProjectionMatrix

Computes the light projection matrix for a point light.
========================
*/
static float R_ComputePointLightProjectionMatrix(idRenderLightLocal * light, idRenderMatrix & localProject) {
	assert(light->parms.pointLight);

	// A point light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[0][0] = 0.5f / light->parms.lightRadius[0];
	localProject[1][1] = 0.5f / light->parms.lightRadius[1];
	localProject[2][2] = 0.5f / light->parms.lightRadius[2];
	localProject[0][3] = 0.5f;
	localProject[1][3] = 0.5f;
	localProject[2][3] = 0.5f;
	localProject[3][3] = 1.0f;	// identity perspective

	return 1.0f;
}

static const float SPOT_LIGHT_MIN_Z_NEAR = 8.0f;
static const float SPOT_LIGHT_MIN_Z_FAR = 16.0f;

/*
========================
R_ComputeSpotLightProjectionMatrix

Computes the light projection matrix for a spot light.
========================
*/
static float R_ComputeSpotLightProjectionMatrix(idRenderLightLocal * light, idRenderMatrix & localProject) {
	const float targetDistSqr = light->parms.target.LengthSqr();
	const float invTargetDist = idMath::InvSqrt(targetDistSqr);
	const float targetDist = invTargetDist * targetDistSqr;

	const idVec3 normalizedTarget = light->parms.target * invTargetDist;
	const idVec3 normalizedRight = light->parms.right * (0.5f * targetDist / light->parms.right.LengthSqr());
	const idVec3 normalizedUp = light->parms.up * (-0.5f * targetDist / light->parms.up.LengthSqr());

	localProject[0][0] = normalizedRight[0];
	localProject[0][1] = normalizedRight[1];
	localProject[0][2] = normalizedRight[2];
	localProject[0][3] = 0.0f;

	localProject[1][0] = normalizedUp[0];
	localProject[1][1] = normalizedUp[1];
	localProject[1][2] = normalizedUp[2];
	localProject[1][3] = 0.0f;

	localProject[3][0] = normalizedTarget[0];
	localProject[3][1] = normalizedTarget[1];
	localProject[3][2] = normalizedTarget[2];
	localProject[3][3] = 0.0f;

	// Set the falloff vector.
	// This is similar to the Z calculation for depth buffering, which means that the
	// mapped texture is going to be perspective distorted heavily towards the zero end.
	const float zNear = Max(light->parms.start * normalizedTarget, SPOT_LIGHT_MIN_Z_NEAR);
	const float zFar = Max(light->parms.end * normalizedTarget, SPOT_LIGHT_MIN_Z_FAR);
	const float zScale = (zNear + zFar) / zFar;

	localProject[2][0] = normalizedTarget[0] * zScale;
	localProject[2][1] = normalizedTarget[1] * zScale;
	localProject[2][2] = normalizedTarget[2] * zScale;
	localProject[2][3] = -zNear * zScale;

	// now offset to the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range
	idVec4 projectedTarget;
	localProject.TransformPoint(light->parms.target, projectedTarget);

	const float ofs0 = 0.5f - projectedTarget[0] / projectedTarget[3];
	localProject[0][0] += ofs0 * localProject[3][0];
	localProject[0][1] += ofs0 * localProject[3][1];
	localProject[0][2] += ofs0 * localProject[3][2];
	localProject[0][3] += ofs0 * localProject[3][3];

	const float ofs1 = 0.5f - projectedTarget[1] / projectedTarget[3];
	localProject[1][0] += ofs1 * localProject[3][0];
	localProject[1][1] += ofs1 * localProject[3][1];
	localProject[1][2] += ofs1 * localProject[3][2];
	localProject[1][3] += ofs1 * localProject[3][3];

	return 1.0f / (zNear + zFar);
}

/*
========================
R_ComputeParallelLightProjectionMatrix

Computes the light projection matrix for a parallel light.
========================
*/
static float R_ComputeParallelLightProjectionMatrix(idRenderLightLocal * light, idRenderMatrix & localProject) {
	assert(light->parms.parallel);

	// A parallel light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[0][0] = 0.5f / light->parms.lightRadius[0];
	localProject[1][1] = 0.5f / light->parms.lightRadius[1];
	localProject[2][2] = 0.5f / light->parms.lightRadius[2];
	localProject[0][3] = 0.5f;
	localProject[1][3] = 0.5f;
	localProject[2][3] = 0.5f;
	localProject[3][3] = 1.0f;	// identity perspective

	return 1.0f;
}

/*
=================
R_CompareViewFrustum
=================
*/
bool R_CompareViewFrustum(idPlane frustum[6], idPlane frustum2[6]) {
	for(int i = 0; i < 6; i++) {
		if(frustum[i] != frustum2[i]) {
			return false;
		}
	}
	return true;
}

/*
=================
R_DeriveLightData

Fills everything in based on light->parms
=================
*/
void R_DeriveLightData( idRenderLightLocal *light ) {
	int i;

	// set the projection
	if ( !light->parms.pointLight ) {
		// projected light

		R_SetLightProject( light->lightProject, vec3_origin /* light->parms.origin */, light->parms.target, 
			light->parms.right, light->parms.up, light->parms.start, light->parms.end);
	} else {
		// point light
		memset( light->lightProject, 0, sizeof( light->lightProject ) );
		light->lightProject[0][0] = 0.5f / light->parms.lightRadius[0];
		light->lightProject[1][1] = 0.5f / light->parms.lightRadius[1];
		light->lightProject[3][2] = 0.5f / light->parms.lightRadius[2];
		light->lightProject[0][3] = 0.5f;
		light->lightProject[1][3] = 0.5f;
		light->lightProject[2][3] = 1.0f;
		light->lightProject[3][3] = 0.5f;
	}

	// set the frustum planes
	R_SetLightFrustum( light->lightProject, light->frustum );

	// rotate the light planes and projections by the axis
	R_AxisToModelMatrix( light->parms.axis, light->parms.origin, light->modelMatrix );

	for ( i = 0 ; i < 6 ; i++ ) {
		idPlane		temp;
		temp = light->frustum[i];
		R_LocalPlaneToGlobal( light->modelMatrix, temp, light->frustum[i] );
	}
	for ( i = 0 ; i < 4 ; i++ ) {
		idPlane		temp;
		temp = light->lightProject[i];
		R_LocalPlaneToGlobal( light->modelMatrix, temp, light->lightProject[i] );
	}

	// adjust global light origin for off center projections and parallel projections
	// we are just faking parallel by making it a very far off center for now
	if ( light->parms.parallel ) {
		idVec3	dir;

		dir = light->parms.lightCenter;
		if ( !dir.Normalize() ) {
			// make point straight up if not specified
			dir[2] = 1;
		}
		light->globalLightOrigin = light->parms.origin + dir * 100000;
	} else {
		light->globalLightOrigin = light->parms.origin + light->parms.axis * light->parms.lightCenter;
	}

	if (light->frustumTris == NULL || R_CompareViewFrustum(light->previousFrustum, light->frustum))
	{
		R_FreeLightDefFrustum(light);

		light->frustumTris = R_PolytopeSurface(6, light->frustum, light->frustumWindings);
		
		for (int i = 0; i < 6; i++) {
			light->previousFrustum[i] = light->frustum[i];
		}
	}

	idRenderMatrix localProject;
	float zScale = 1.0f;
	if (light->parms.parallel) {
		zScale = R_ComputeParallelLightProjectionMatrix(light, localProject);
	}
	else if (light->parms.pointLight) {
		zScale = R_ComputePointLightProjectionMatrix(light, localProject);
	}
	else {
		zScale = R_ComputeSpotLightProjectionMatrix(light, localProject);
	}

	// Rotate and translate the light projection by the light matrix.
	// 99% of lights remain axis aligned in world space.
	idRenderMatrix lightMatrix;
	idRenderMatrix::CreateFromOriginAxis(light->parms.origin, light->parms.axis, lightMatrix);

	idRenderMatrix inverseLightMatrix;
	if (!idRenderMatrix::Inverse(lightMatrix, inverseLightMatrix)) {
		idLib::Warning("lightMatrix invert failed");
	}

	// 'baseLightProject' goes from global space -> light local space -> light projective space
	idRenderMatrix::Multiply(localProject, inverseLightMatrix, light->baseLightProject);

	// Invert the light projection so we can deform zero-to-one cubes into
	// the light model and calculate global bounds.
	if (!idRenderMatrix::Inverse(light->baseLightProject, light->inverseBaseLightProject)) {
		idLib::Warning("baseLightProject invert failed");
	}

	// calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
	idRenderMatrix::ProjectedBounds(light->globalLightBounds, light->inverseBaseLightProject, bounds_zeroOneCube, false);
}

/*
=================
R_CreateLightRefs
=================
*/
#define	MAX_LIGHT_VERTS	40
void R_CreateLightRefs( idRenderLightLocal *light ) {
	idVec3	points[MAX_LIGHT_VERTS];
	int		i;
	srfTriangles_t	*tri;

	tri = light->frustumTris;

	// because a light frustum is made of only six intersecting planes,
	// we should never be able to get a stupid number of points...
	if ( tri->numVerts > MAX_LIGHT_VERTS ) {
		common->Error( "R_CreateLightRefs: %i points in frustumTris!", tri->numVerts );
	}
	for ( i = 0 ; i < tri->numVerts ; i++ ) {
		points[i] = tri->verts[i].xyz;
	}

	if (  r_showUpdates.GetBool() && ( tri->bounds[1][0] - tri->bounds[0][0] > 1024 ||
		tri->bounds[1][1] - tri->bounds[0][1] > 1024 ) ) {
		common->Printf( "big lightRef: %f,%f\n", tri->bounds[1][0] - tri->bounds[0][0]
			,tri->bounds[1][1] - tri->bounds[0][1] );
	}

	// determine the areaNum for the light origin, which may let us
	// cull the light if it is behind a closed door
	// it is debatable if we want to use the entity origin or the center offset origin,
	// but we definitely don't want to use a parallel offset origin
	light->areaNum = light->world->PointInArea( light->globalLightOrigin );
	if ( light->areaNum == -1 ) {
		light->areaNum = light->world->PointInArea( light->parms.origin );
	}

	// bump the view count so we can tell if an
	// area already has a reference
	tr.viewCount++;

	// if we have a prelight model that includes all the shadows for the major world occluders,
	// we can limit the area references to those visible through the portals from the light center.
	// We can't do this in the normal case, because shadows are cast from back facing triangles, which
	// may be in areas not directly visible to the light projection center.
	//if ( light->parms.prelightModel && r_useLightPortalFlow.GetBool() && light->lightShader->LightCastsShadows() ) {
	//	light->world->FlowLightThroughPortals( light );
	//} else {
	//	// push these points down the BSP tree into areas
	//	light->world->PushVolumeIntoTree( NULL, light, tri->numVerts, points );
	//}
}

/*
===============
R_RenderLightFrustum

Called by the editor and dmap to operate on light volumes
===============
*/
void R_RenderLightFrustum( const idRenderLightParms&renderLight, idPlane lightFrustum[6] ) {
	idRenderLightLocal	fakeLight;

	memset( &fakeLight, 0, sizeof( fakeLight ) );
	fakeLight.parms = renderLight;

	R_DeriveLightData( &fakeLight );
	
	R_FreeStaticTriSurf( fakeLight.frustumTris );

	for ( int i = 0 ; i < 6 ; i++ ) {
		lightFrustum[i] = fakeLight.frustum[i];
	}
}


//=================================================================================

/*
===============
WindingCompletelyInsideLight
===============
*/
bool WindingCompletelyInsideLight( const idWinding *w, const idRenderLightLocal *ldef ) {
	int		i, j;

	for ( i = 0 ; i < w->GetNumPoints() ; i++ ) {
		for ( j = 0 ; j < 6 ; j++ ) {
			float	d;

			d = (*w)[i].ToVec3() * ldef->frustum[j].Normal() + ldef->frustum[j][3];
			if ( d > 0 ) {
				return false;
			}
		}
	}
	return true;
}

/*
======================
R_CreateLightDefFogPortals

When a fog light is created or moved, see if it completely
encloses any portals, which may allow them to be fogged closed.
======================
*/
void R_CreateLightDefFogPortals( idRenderLightLocal *ldef ) {
	
}

/*
====================
R_FreeLightDefDerivedData

Frees all references and lit surfaces from the light
====================
*/
void R_FreeLightDefDerivedData( idRenderLightLocal *ldef ) {
	// free all the interactions
	while ( ldef->firstInteraction != NULL ) {
		ldef->firstInteraction->UnlinkAndFree();
	}
}

/*
===================
R_FreeEntityDefDerivedData

Used by both RE_FreeEntityDef and RE_UpdateEntityDef
Does not actually free the entityDef.
===================
*/
void R_FreeEntityDefDerivedData( idRenderEntityLocal *def, bool keepDecals, bool keepCachedDynamicModel ) {
	int i;

	// demo playback needs to free the joints, while normal play
	// leaves them in the control of the game
	if ( session->readDemo ) {
		if ( def->parms.joints ) {
			Mem_Free16( def->parms.joints );
			def->parms.joints = NULL;
		}
		if ( def->parms.callbackData ) {
			Mem_Free( def->parms.callbackData );
			def->parms.callbackData = NULL;
		}
		for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
			if ( def->parms.gui[ i ] ) {
				delete def->parms.gui[ i ];
				def->parms.gui[ i ] = NULL;
			}
		}
	}

	// free all the interactions
	while ( def->firstInteraction != NULL ) {
		def->firstInteraction->UnlinkAndFree();
	}

	// clear the dynamic model if present
	if ( def->dynamicModel ) {
		def->dynamicModel = NULL;
	}

	if ( !keepDecals ) {
		R_FreeEntityDefDecals( def );
		R_FreeEntityDefOverlay( def );
	}

	if ( !keepCachedDynamicModel ) {
		delete def->cachedDynamicModel;
		def->cachedDynamicModel = NULL;
	}
}

/*
==================
R_ClearEntityDefDynamicModel

If we know the reference bounds stays the same, we
only need to do this on entity update, not the full
R_FreeEntityDefDerivedData
==================
*/
void R_ClearEntityDefDynamicModel( idRenderEntityLocal *def ) {
	// free all the interaction surfaces
	for( idInteraction *inter = def->firstInteraction; inter != NULL && !inter->IsEmpty(); inter = inter->entityNext ) {
		inter->FreeSurfaces();
	}

	// clear the dynamic model if present
	if ( def->dynamicModel ) {
		def->dynamicModel = NULL;
	}
}

/*
===================
R_FreeEntityDefDecals
===================
*/
void R_FreeEntityDefDecals( idRenderEntityLocal *def ) {
	while( def->decals ) {
		idRenderModelDecal *next = def->decals->Next();
		idRenderModelDecal::Free( def->decals );
		def->decals = next;
	}
}

/*
===================
R_FreeEntityDefFadedDecals
===================
*/
void R_FreeEntityDefFadedDecals( idRenderEntityLocal *def, int time ) {
	def->decals = idRenderModelDecal::RemoveFadedDecals( def->decals, time );
}

/*
===================
R_FreeEntityDefOverlay
===================
*/
void R_FreeEntityDefOverlay( idRenderEntityLocal *def ) {
	if ( def->overlay ) {
		idRenderModelOverlay::Free( def->overlay );
		def->overlay = NULL;
	}
}

/*
===================
R_FreeDerivedData

ReloadModels and RegenerateWorld call this
// FIXME: need to do this for all worlds
===================
*/
void R_FreeDerivedData( void ) {
	int i, j;
	idRenderWorldLocal *rw;
	idRenderEntityLocal *def;
	idRenderLightLocal *light;

	for ( j = 0; j < tr.worlds.Num(); j++ ) {
		rw = tr.worlds[j];

		for ( i = 0; i < rw->entityDefs.Num(); i++ ) {
			def = rw->entityDefs[i];
			if ( !def ) {
				continue;
			}
			R_FreeEntityDefDerivedData( def, false, false );
		}

		for ( i = 0; i < rw->lightDefs.Num(); i++ ) {
			light = rw->lightDefs[i];
			if ( !light ) {
				continue;
			}
			R_FreeLightDefDerivedData( light );
		}
	}
}

/*
===================
R_CheckForEntityDefsUsingModel
===================
*/
void R_CheckForEntityDefsUsingModel( idRenderModel *model ) {
	int i, j;
	idRenderWorldLocal *rw;
	idRenderEntityLocal	*def;

	for ( j = 0; j < tr.worlds.Num(); j++ ) {
		rw = tr.worlds[j];

		for ( i = 0 ; i < rw->entityDefs.Num(); i++ ) {
			def = rw->entityDefs[i];
			if ( !def ) {
				continue;
			}
			if ( def->parms.hModel == model ) {
				//assert( 0 );
				// this should never happen but Radiant messes it up all the time so just free the derived data
				R_FreeEntityDefDerivedData( def, false, false );
			}
		}
	}
}

/*
===================
R_ReCreateWorldReferences

ReloadModels and RegenerateWorld call this
// FIXME: need to do this for all worlds
===================
*/
void R_ReCreateWorldReferences( void ) {
	int i, j;
	idRenderWorldLocal *rw;
	idRenderEntityLocal *def;
	idRenderLightLocal *light;

	// let the interaction generation code know this shouldn't be optimized for
	// a particular view
	tr.viewDef = NULL;

	for ( j = 0; j < tr.worlds.Num(); j++ ) {
		rw = tr.worlds[j];

		for ( i = 0 ; i < rw->entityDefs.Num() ; i++ ) {
			def = rw->entityDefs[i];
			if ( !def ) {
				continue;
			}
			R_CreateEntityRefs(def);
		}

// jmarshall - why is this needed?
		//for ( i = 0 ; i < rw->lightDefs.Num() ; i++ ) {
		//	light = rw->lightDefs[i];
		//	if ( !light ) {
		//		continue;
		//	}			
		//
		//	idRenderLightParms parms = light->parms;
		//
		//	light->world->FreeLightDef( i );
		//	rw->UpdateLightDef( i, &parms );
		//}
// jmarshall end
	}
}

/*
===================
R_RegenerateWorld_f

Frees and regenerates all references and interactions, which
must be done when switching between display list mode and immediate mode
===================
*/
void R_RegenerateWorld_f( const idCmdArgs &args ) {
	R_FreeDerivedData();

	// watch how much memory we allocate
	tr.staticAllocCount = 0;

	R_ReCreateWorldReferences();

	common->Printf( "Regenerated world, staticAllocCount = %i.\n", tr.staticAllocCount );
}
