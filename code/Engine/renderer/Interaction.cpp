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
#include "../model/Model_local.h"

/*
===========================================================================

idInteraction implementation

===========================================================================
*/

// FIXME: use private allocator for srfCullInfo_t

/*
=====================
R_CalcInteractionCullBits

We want to cull a little on the sloppy side, because the pre-clipping
of geometry to the lights in dmap will give many cases that are right
at the border we throw things out on the border, because if any one
vertex is clearly inside, the entire triangle will be accepted.
=====================
*/
void R_CalcInteractionCullBits( const idRenderEntityLocal *ent, const srfTriangles_t *tri, const idRenderLightLocal *light, srfCullInfo_t &cullInfo ) {
	int i, frontBits;

	if ( cullInfo.cullBits != NULL ) {
		return;
	}

	frontBits = 0;

	// cull the triangle surface bounding box
	for ( i = 0; i < 6; i++ ) {

		R_GlobalPlaneToLocal( ent->modelMatrix, -light->frustum[i], cullInfo.localClipPlanes[i] );

		// get front bits for the whole surface
		if ( tri->bounds.PlaneDistance( cullInfo.localClipPlanes[i] ) >= LIGHT_CLIP_EPSILON ) {
			frontBits |= 1<<i;
		}
	}

	// if the surface is completely inside the light frustum
	if ( frontBits == ( ( 1 << 6 ) - 1 ) ) {
		cullInfo.cullBits = LIGHT_CULL_ALL_FRONT;
		return;
	}

	cullInfo.cullBits = (byte *) R_StaticAlloc( tri->numVerts * sizeof( cullInfo.cullBits[0] ) );
	SIMDProcessor->Memset( cullInfo.cullBits, 0, tri->numVerts * sizeof( cullInfo.cullBits[0] ) );

	float *planeSide = (float *) _alloca16( tri->numVerts * sizeof( float ) );

	for ( i = 0; i < 6; i++ ) {
		// if completely infront of this clipping plane
		if ( frontBits & ( 1 << i ) ) {
			continue;
		}
		SIMDProcessor->Dot( planeSide, cullInfo.localClipPlanes[i], tri->verts, tri->numVerts );
		SIMDProcessor->CmpLT( cullInfo.cullBits, i, planeSide, LIGHT_CLIP_EPSILON, tri->numVerts );
	}
}

/*
================
R_FreeInteractionCullInfo
================
*/
void R_FreeInteractionCullInfo( srfCullInfo_t &cullInfo ) {
	if ( cullInfo.facing != NULL ) {
		R_StaticFree( cullInfo.facing );
		cullInfo.facing = NULL;
	}
	if ( cullInfo.cullBits != NULL ) {
		if ( cullInfo.cullBits != LIGHT_CULL_ALL_FRONT ) {
			R_StaticFree( cullInfo.cullBits );
		}
		cullInfo.cullBits = NULL;
	}
}

#define	MAX_CLIPPED_POINTS	20
typedef struct {
	int		numVerts;
	idVec3	verts[MAX_CLIPPED_POINTS];
} clipTri_t;

/*
=============
R_ChopWinding

Clips a triangle from one buffer to another, setting edge flags
The returned buffer may be the same as inNum if no clipping is done
If entirely clipped away, clipTris[returned].numVerts == 0

I have some worries about edge flag cases when polygons are clipped
multiple times near the epsilon.
=============
*/
static int R_ChopWinding( clipTri_t clipTris[2], int inNum, const idPlane plane ) {
	clipTri_t	*in, *out;
	float	dists[MAX_CLIPPED_POINTS];
	int		sides[MAX_CLIPPED_POINTS];
	int		counts[3];
	float	dot;
	int		i, j;
	idVec3	mid;
	bool	front;

	in = &clipTris[inNum];
	out = &clipTris[inNum^1];
	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	front = false;
	for ( i = 0; i < in->numVerts; i++ ) {
		dot = in->verts[i] * plane.Normal() + plane[3];
		dists[i] = dot;
		if ( dot < LIGHT_CLIP_EPSILON ) {	// slop onto the back
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_FRONT;
			if ( dot > LIGHT_CLIP_EPSILON ) {
				front = true;
			}
		}
		counts[sides[i]]++;
	}

	// if none in front, it is completely clipped away
	if ( !front ) {
		in->numVerts = 0;
		return inNum;
	}
	if ( !counts[SIDE_BACK] ) {
		return inNum;		// inout stays the same
	}

	// avoid wrapping checks by duplicating first value to end
	sides[i] = sides[0];
	dists[i] = dists[0];
	in->verts[in->numVerts] = in->verts[0];

	out->numVerts = 0;
	for ( i = 0 ; i < in->numVerts ; i++ ) {
		idVec3 &p1 = in->verts[i];
		
		if ( sides[i] == SIDE_FRONT ) {
			out->verts[out->numVerts] = p1;
			out->numVerts++;
		}

		if ( sides[i+1] == sides[i] ) {
			continue;
		}
			
		// generate a split point
		idVec3 &p2 = in->verts[i+1];
		
		dot = dists[i] / ( dists[i] - dists[i+1] );
		for ( j = 0; j < 3; j++ ) {
			mid[j] = p1[j] + dot * ( p2[j] - p1[j] );
		}
			
		out->verts[out->numVerts] = mid;

		out->numVerts++;
	}

	return inNum ^ 1;
}

/*
===================
R_ClipTriangleToLight

Returns false if nothing is left after clipping
===================
*/
static bool	R_ClipTriangleToLight( const idVec3 &a, const idVec3 &b, const idVec3 &c, int planeBits, const idPlane frustum[6] ) {
	int			i;
	clipTri_t	pingPong[2];
	int			p;

	pingPong[0].numVerts = 3;
	pingPong[0].verts[0] = a;
	pingPong[0].verts[1] = b;
	pingPong[0].verts[2] = c;

	p = 0;
	for ( i = 0 ; i < 6 ; i++ ) {
		if ( planeBits & ( 1 << i ) ) {
			p = R_ChopWinding( pingPong, p, frustum[i] );
			if ( pingPong[p].numVerts < 1 ) {
				return false;
			}
		}
	}

	return true;
}

/*
===============
idInteraction::idInteraction
===============
*/
idInteraction::idInteraction( void ) {
	numSurfaces				= 0;
	lightChannel			= 0;
	entityDef				= NULL;
	lightDef				= NULL;
	lightNext				= NULL;
	lightPrev				= NULL;
	entityNext				= NULL;
	entityPrev				= NULL;
	dynamicModelFrameCount	= 0;
	frustumState			= FRUSTUM_UNINITIALIZED;
	frustumAreas			= NULL;
}

/*
===============
idInteraction::AllocAndLink
===============
*/
idInteraction *idInteraction::AllocAndLink( idRenderEntityLocal *edef, idRenderLightLocal *ldef ) {
	if ( !edef || !ldef ) {
		common->Error( "idInteraction::AllocAndLink: NULL parm" );
	}

	idRenderWorldLocal *renderWorld = edef->world;

	idInteraction *interaction = renderWorld->interactionAllocator.Alloc();

	// link and initialize
	interaction->dynamicModelFrameCount = 0;

	interaction->lightDef = ldef;
	interaction->entityDef = edef;

	interaction->interactionClass = ldef->parms.classType;

	interaction->lightChannel = edef->parms.lightChannel;

	// World meshes are only on channel LIGHT_CHANNEL_WORLD.
	if(edef->parms.hModel->IsWorldMesh()) {
		interaction->lightChannel |= 1 << LIGHT_CHANNEL_WORLD;
	}

	interaction->numSurfaces = -1;		// not checked yet
	//interaction->surfaces = NULL;

	interaction->frustumState = idInteraction::FRUSTUM_UNINITIALIZED;
	interaction->frustumAreas = NULL;

	// link at the start of the entity's list
	interaction->lightNext = ldef->firstInteraction;
	interaction->lightPrev = NULL;
	ldef->firstInteraction = interaction;
	if ( interaction->lightNext != NULL ) {
		interaction->lightNext->lightPrev = interaction;
	} else {
		ldef->lastInteraction = interaction;
	}

	// link at the start of the light's list
	interaction->entityNext = edef->firstInteraction;
	interaction->entityPrev = NULL;
	edef->firstInteraction = interaction;
	if ( interaction->entityNext != NULL ) {
		interaction->entityNext->entityPrev = interaction;
	} else {
		edef->lastInteraction = interaction;
	}

	// update the interaction table
	if ( renderWorld->interactionTable ) {
		int index = ldef->index * renderWorld->interactionTableWidth + edef->index;
		if ( renderWorld->interactionTable[index] != NULL ) {
			common->Error( "idInteraction::AllocAndLink: non NULL table entry" );
		}
		renderWorld->interactionTable[ index ] = interaction;
	}

	return interaction;
}

/*
===============
idInteraction::FreeSurfaces

Frees the surfaces, but leaves the interaction linked in, so it
will be regenerated automatically
===============
*/
void idInteraction::FreeSurfaces( void ) {
	for (int i = 0; i < this->numSurfaces; i++) {
		surfaceInteraction_t* sint = &this->surfaces[i];

		R_FreeInteractionCullInfo(sint->cullInfo);
	}

	surfaces.Clear();

	this->numSurfaces = -1;
}

/*
===============
idInteraction::Unlink
===============
*/
void idInteraction::Unlink( void ) {

	// unlink from the entity's list
	if ( this->entityPrev ) {
		this->entityPrev->entityNext = this->entityNext;
	} else {
		this->entityDef->firstInteraction = this->entityNext;
	}
	if ( this->entityNext ) {
		this->entityNext->entityPrev = this->entityPrev;
	} else {
		this->entityDef->lastInteraction = this->entityPrev;
	}
	this->entityNext = this->entityPrev = NULL;

	// unlink from the light's list
	if ( this->lightPrev ) {
		this->lightPrev->lightNext = this->lightNext;
	} else {
		this->lightDef->firstInteraction = this->lightNext;
	}
	if ( this->lightNext ) {
		this->lightNext->lightPrev = this->lightPrev;
	} else {
		this->lightDef->lastInteraction = this->lightPrev;
	}
	this->lightNext = this->lightPrev = NULL;
}

/*
===============
idInteraction::UnlinkAndFree

Removes links and puts it back on the free list.
===============
*/
void idInteraction::UnlinkAndFree( void ) {

	// clear the table pointer
	idRenderWorldLocal *renderWorld = this->lightDef->world;
	if ( renderWorld->interactionTable ) {
		int index = this->lightDef->index * renderWorld->interactionTableWidth + this->entityDef->index;
		if ( renderWorld->interactionTable[index] != this ) {
			common->Error( "idInteraction::UnlinkAndFree: interactionTable wasn't set" );
		}
		renderWorld->interactionTable[index] = NULL;
	}

	Unlink();

	FreeSurfaces();

	// free the interaction area references
	areaNumRef_t *area, *nextArea;
	for ( area = frustumAreas; area; area = nextArea ) {
		nextArea = area->next;
		renderWorld->areaNumRefAllocator.Free( area );
	}

	// put it back on the free list
	renderWorld->interactionAllocator.Free( this );
}

/*
=========================
idInteraction::HasLightChannel
=========================
*/
bool idInteraction::HasLightChannel(int lightChannel) {
	return (!!((this->lightChannel) & (1ULL << (lightChannel))));
}

/*
===============
idInteraction::MakeEmpty

Makes the interaction empty and links it at the end of the entity's and light's interaction lists.
===============
*/
void idInteraction::MakeEmpty( void ) {

	// an empty interaction has no surfaces
	numSurfaces = 0;

	Unlink();

	// relink at the end of the entity's list
	this->entityNext = NULL;
	this->entityPrev = this->entityDef->lastInteraction;
	this->entityDef->lastInteraction = this;
	if ( this->entityPrev ) {
		this->entityPrev->entityNext = this;
	} else {
		this->entityDef->firstInteraction = this;
	}

	// relink at the end of the light's list
	this->lightNext = NULL;
	this->lightPrev = this->lightDef->lastInteraction;
	this->lightDef->lastInteraction = this;
	if ( this->lightPrev ) {
		this->lightPrev->lightNext = this;
	} else {
		this->lightDef->firstInteraction = this;
	}
}

/*
===============
idInteraction::HasShadows
===============
*/
ID_INLINE bool idInteraction::HasShadows( void ) const {
	return ( !lightDef->parms.noShadows && !entityDef->parms.noShadow );
}

/*
===============
idInteraction::MemoryUsed

Counts up the memory used by all the surfaceInteractions, which
will be used to determine when we need to start purging old interactions.
===============
*/
int idInteraction::MemoryUsed( void ) {
	int		total = 0;

	//for ( int i = 0 ; i < numSurfaces ; i++ ) {
	//	surfaceInteraction_t *inter = &surfaces[i];
	//
	//	total += R_TriSurfMemory( inter->shadowTris );
	//}

	return total;
}

/*
==================
idInteraction::CalcInteractionScissorRectangle
==================
*/
idScreenRect idInteraction::CalcInteractionScissorRectangle( const idFrustum &viewFrustum ) {
// jmarshall - interaction scissors fix me!
	return lightDef->viewLight->scissorRect;
/*
	idScreenRect	scissorRect;

	if ( r_useInteractionScissors.GetInteger() == 0 ) {
		return lightDef->viewLight->scissorRect;
	}

	//if ( r_useInteractionScissors.GetInteger() < 0 ) {
	//	// this is the code from Cass at nvidia, it is more precise, but slower
	//	return R_CalcIntersectionScissor( lightDef, entityDef, tr.viewDef );
	//}

	// the following is Mr.E's code

	// frustum must be initialized and valid
	if ( frustumState == idInteraction::FRUSTUM_UNINITIALIZED || frustumState == idInteraction::FRUSTUM_INVALID ) {
		return lightDef->viewLight->scissorRect;
	}

	// calculate bounds of the interaction frustum projected into the view frustum
	if ( lightDef->parms.pointLight ) {
		viewFrustum.ClippedProjectionBounds( frustum, idBox( lightDef->parms.origin, lightDef->parms.lightRadius, lightDef->parms.axis ), projectionBounds );
	} else {
		viewFrustum.ClippedProjectionBounds( frustum, idBox( lightDef->frustumTris->bounds ), projectionBounds );
	}

	// derive a scissor rectangle from the projection bounds
	scissorRect = R_ScreenRectFromViewFrustumBounds( projectionBounds );

	// intersect with the portal crossing scissor rectangle
	scissorRect.Intersect( portalRect );

	if ( r_showInteractionScissors.GetInteger() > 0 ) {
		R_ShowColoredScreenRect( scissorRect, lightDef->index );
	}

	return scissorRect;
*/
// jmarshall end
}

/*
===================
idInteraction::CullInteractionByViewFrustum
===================
*/
bool idInteraction::CullInteractionByViewFrustum( const idFrustum &viewFrustum ) {

	if ( !r_useInteractionCulling.GetBool() ) {
		return false;
	}

	if ( frustumState == idInteraction::FRUSTUM_INVALID ) {
		return false;
	}

	if ( frustumState == idInteraction::FRUSTUM_UNINITIALIZED ) {

		frustum.FromProjection( idBox( entityDef->referenceBounds, entityDef->parms.origin, entityDef->parms.axis ), lightDef->globalLightOrigin, MAX_WORLD_SIZE );

		if ( !frustum.IsValid() ) {
			frustumState = idInteraction::FRUSTUM_INVALID;
			return false;
		}

		if ( lightDef->parms.pointLight ) {
			frustum.ConstrainToBox( idBox( lightDef->parms.origin, lightDef->parms.lightRadius, lightDef->parms.axis ) );
		} else {
			frustum.ConstrainToBox( idBox( lightDef->frustumTris->bounds ) );
		}

		frustumState = idInteraction::FRUSTUM_VALID;
	}

	if ( !viewFrustum.IntersectsFrustum( frustum ) ) {
		return true;
	}

	if ( r_showInteractionFrustums.GetInteger() ) {
		static idVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };
		tr.viewDef->renderWorld->DebugFrustum( colors[lightDef->index & 7], frustum, ( r_showInteractionFrustums.GetInteger() > 1 ) );
		if ( r_showInteractionFrustums.GetInteger() > 2 ) {
			tr.viewDef->renderWorld->DebugBox( colorWhite, idBox( entityDef->referenceBounds, entityDef->parms.origin, entityDef->parms.axis ) );
		}
	}

	return false;
}

/*
====================
idInteraction::CreateInteraction

Called when a entityDef and a lightDef are both present in a
portalArea, and might be visible.  Performs cull checking before doing the expensive
computations.

References tr.viewCount so lighting surfaces will only be created if the ambient surface is visible,
otherwise it will be marked as deferred.

The results of this are cached and valid until the light or entity change.
====================
*/
void idInteraction::CreateInteraction( const idRenderModel *model ) {
	const idMaterial*	shader;
	bool				interactionGenerated;
	idBounds			bounds;
	viewLight_t* vLight;
	viewEntity_t* vEntity;

	vLight = lightDef->viewLight;
	vEntity = entityDef->viewEntity;
	 
	tr.pc.c_createInteractions++;

	bounds = model->Bounds( entityDef );

// jmarshall We don't need exact frustum checks its too slow.
	// if it doesn't contact the light frustum, none of the surfaces will
	//if ( R_CullLocalBox( bounds, entityDef->modelMatrix, 6, lightDef->frustum ) ) {
	//	MakeEmpty();
	//	return;
	//}
	hasSkinning = false;
	bounds += entityDef->parms.origin;
	if(!bounds.IntersectsBounds(lightDef->globalLightBounds)) {
		MakeEmpty();
		return;
	}
// jmarshall end

	//
	// create slots for each of the model's surfaces
	//
	numSurfaces = model->NumSurfaces();
	surfaces.SetNum(numSurfaces);
	memset(surfaces.Ptr(), 0, sizeof(surfaceInteraction_t) * numSurfaces);

	interactionGenerated = false;

	// check each surface in the model
	for ( int c = 0 ; c < model->NumSurfaces() ; c++ ) {
		const modelSurface_t	*surf;
		srfTriangles_t	*tri;
	
		surf = model->Surface( c );

		tri = surf->geometry;
		if ( !tri ) {
			continue;
		}

		// determine the shader for this surface, possibly by skinning
		shader = surf->shader;
		shader = R_RemapShaderBySkin( shader, entityDef->parms.customSkin, entityDef->parms.customShader );
		if(entityDef->parms.customShader != NULL) {
			shader = shader;
		}

		if ( !shader ) {
			continue;
		}

// jmarshall - no need to test each surface.
		// try to cull each surface
		//if ( R_CullLocalBox( tri->bounds, entityDef->modelMatrix, 6, lightDef->frustum ) ) {
		//	continue;
		//}
// jmarshall end

		surfaceInteraction_t *sint = &surfaces[c];

		sint->shader = shader;	
		sint->scissorRect = vEntity->scissorRect;
		sint->forceVirtualTextureHighQuality = entityDef->parms.forceVirtualTextureHighQuality;

// jmarshall GPU skinning.
		if (model->IsSkeletalMesh())
		{
			const idRenderModelMD5Instance* staticRenderModel = (const idRenderModelMD5Instance*)model;
			sint->skinning.jointBuffer = staticRenderModel->jointBuffer;
			sint->skinning.numInvertedJoints = staticRenderModel->numInvertedJoints;
			sint->skinning.jointsInverted = staticRenderModel->jointsInverted;
			hasSkinning = true;
		}
// jmarshall end

		// save the ambient tri pointer so we can reject lightTri interactions
		// when the ambient surface isn't in view, and we can get shared vertex
		// and shadow data from the source surface
		sint->ambientTris = tri;

		// generate a lighted surface and add it
		if ( shader->ReceivesLighting() ) {
			interactionGenerated = true;
		}

		// free the cull information when it's no longer needed
		R_FreeInteractionCullInfo(sint->cullInfo);
	}

	// if none of the surfaces generated anything, don't even bother checking?
	if ( !interactionGenerated ) {
		MakeEmpty();
	}
}

/*
======================
R_PotentiallyInsideInfiniteShadow

If we know that we are "off to the side" of an infinite shadow volume,
we can draw it without caps in zpass mode
======================
*/
static bool R_PotentiallyInsideInfiniteShadow( const srfTriangles_t *occluder,
											  const idVec3 &localView, const idVec3 &localLight ) {
	idBounds	exp;

	// expand the bounds to account for the near clip plane, because the
	// view could be mathematically outside, but if the near clip plane
	// chops a volume edge, the zpass rendering would fail.
	float	znear = r_znear.GetFloat();
	if ( tr.viewDef->renderView.cramZNear ) {
		znear *= 0.25f;
	}
	float	stretch = znear * 2;	// in theory, should vary with FOV
	exp[0][0] = occluder->bounds[0][0] - stretch;
	exp[0][1] = occluder->bounds[0][1] - stretch;
	exp[0][2] = occluder->bounds[0][2] - stretch;
	exp[1][0] = occluder->bounds[1][0] + stretch;
	exp[1][1] = occluder->bounds[1][1] + stretch;
	exp[1][2] = occluder->bounds[1][2] + stretch;

	if ( exp.ContainsPoint( localView ) ) {
		return true;
	}
	if ( exp.ContainsPoint( localLight ) ) {
		return true;
	}

	// if the ray from localLight to localView intersects a face of the
	// expanded bounds, we will be inside the projection

	idVec3	ray = localView - localLight;

	// intersect the ray from the view to the light with the near side of the bounds
	for ( int axis = 0; axis < 3; axis++ ) {
		float	d, frac;
		idVec3	hit;

		if ( localLight[axis] < exp[0][axis] ) {
			if ( localView[axis] < exp[0][axis] ) {
				continue;
			}
			d = exp[0][axis] - localLight[axis];
			frac = d / ray[axis];
			hit = localLight + frac * ray;
			hit[axis] = exp[0][axis];
		} else if ( localLight[axis] > exp[1][axis] ) {
			if ( localView[axis] > exp[1][axis] ) {
				continue;
			}
			d = exp[1][axis] - localLight[axis];
			frac = d / ray[axis];
			hit = localLight + frac * ray;
			hit[axis] = exp[1][axis];
		} else {
			continue;
		}

		if ( exp.ContainsPoint( hit ) ) {
			return true;
		}
	}

	// the view is definitely not inside the projected shadow
	return false;
}

/*
==================
idInteraction::AddActiveInteraction

Create and add any necessary light and shadow triangles

If the model doesn't have any surfaces that need interactions
with this type of light, it can be skipped, but we might need to
instantiate the dynamic model to find out
==================
*/
void idInteraction::AddActiveInteraction( void ) {
	viewLight_t *	vLight;
	viewEntity_t *	vEntity;
	idScreenRect	shadowScissor;
	idScreenRect	lightScissor;
	idVec3			localLightOrigin;
	idVec3			localViewOrigin;

	vLight = lightDef->viewLight;
	vEntity = entityDef->viewEntity;

	// do not waste time culling the interaction frustum if there will be no shadows
	if ( !HasShadows() ) {

		// use the entity scissor rectangle
		shadowScissor = vEntity->scissorRect;

	// culling does not seem to be worth it for static world models
	} else if ( entityDef->parms.hModel->IsStaticWorldModel() ) {

		// use the light scissor rectangle
		shadowScissor = vLight->scissorRect;

	} else {
// jmarshall - disable, do we need this?
		// try to cull the interaction
		// this will also cull the case where the light origin is inside the
		// view frustum and the entity bounds are outside the view frustum
//		if ( CullInteractionByViewFrustum( tr.viewDef->viewFrustum ) && !tr.viewDef->renderView.skipFrustumInteractionCheck) {
//			return;
//		}
// jmarshall end

		// calculate the shadow scissor rectangle
		shadowScissor = CalcInteractionScissorRectangle( tr.viewDef->viewFrustum );
	}

	// get out before making the dynamic model if the shadow scissor rectangle is empty
	if ( shadowScissor.IsEmpty() ) {
		return;
	}

	// Suppress shadows and meshs surfaces if requested.
	if (!r_skipSuppress.GetBool()) {
		if (vEntity->entityDef->parms.suppressShadowInViewID && vEntity->entityDef->parms.suppressShadowInViewID == tr.viewDef->renderView.viewID) {
			return;
		}
		if (vEntity->entityDef->parms.suppressShadowInLightID && vEntity->entityDef->parms.suppressShadowInLightID == vLight->lightDef->parms.lightId) {
			return;
		}

		if (vEntity->entityDef->parms.suppressSurfaceInViewID
			&& vEntity->entityDef->parms.suppressSurfaceInViewID == tr.viewDef->renderView.viewID) {
			return;
		}
		if (vEntity->entityDef->parms.allowSurfaceInViewID
			&& vEntity->entityDef->parms.allowSurfaceInViewID != tr.viewDef->renderView.viewID) {
			return;
		}
	}

	// We will need the dynamic surface created to make interactions, even if the
	// model itself wasn't visible.  This just returns a cached value after it
	// has been generated once in the view.
	idRenderModel *model = R_EntityDefDynamicModel( entityDef );
	if ( model == NULL || model->NumSurfaces() <= 0 ) {
		return;
	}

	// the dynamic model may have changed since we built the surface list
	if ( !IsDeferred() && entityDef->dynamicModelFrameCount != dynamicModelFrameCount ) {
		FreeSurfaces();
	}
	dynamicModelFrameCount = entityDef->dynamicModelFrameCount;

	// actually create the interaction if needed, building light and shadow surfaces as needed
	if ( IsDeferred() ) {
		CreateInteraction( model );
	}

	R_GlobalPointToLocal( vEntity->modelMatrix, lightDef->globalLightOrigin, localLightOrigin );
	R_GlobalPointToLocal( vEntity->modelMatrix, tr.viewDef->renderView.vieworg, localViewOrigin );

	// calculate the scissor as the intersection of the light and model rects
	// this is used for light triangles, but not for shadow triangles
	lightScissor = vLight->scissorRect;
	lightScissor.Intersect( vEntity->scissorRect );

	bool lightScissorsEmpty = lightScissor.IsEmpty();

	// for each surface of this entity / light interaction
	for ( int i = 0; i < numSurfaces; i++ ) {
		surfaceInteraction_t *sint = &surfaces[i];

		// see if the base surface is visible, we may still need to add shadows even if empty
		if ( !lightScissorsEmpty && sint->ambientTris && sint->ambientTris->ambientViewCount == tr.viewCount ) {

			// make sure we have created this interaction, which may have been deferred
			// on a previous use that only needed the shadow
			R_FreeInteractionCullInfo(sint->cullInfo);

			srfTriangles_t *lightTris = sint->ambientTris;

			if ( lightTris ) {

				// try to cull before adding
				// FIXME: this may not be worthwhile. We have already done culling on the ambient,
				// but individual surfaces may still be cropped somewhat more
// jmarshall - this is costing more then it's saving.
				//if ( !R_CullLocalBox( lightTris->bounds, vEntity->modelMatrix, 5, tr.viewDef->frustum ) ) {
				{
// jmarshall end

					// make sure the original surface has its ambient cache created
					srfTriangles_t *tri = sint->ambientTris;
					if ( !tri->ambientCache ) {
						if ( !R_CreateAmbientCache( tri, sint->shader->ReceivesLighting() ) ) {
							// skip if we were out of vertex memory
							continue;
						}
					}

					// reference the original surface's ambient cache
					lightTris->ambientCache = tri->ambientCache;

					// touch the ambient surface so it won't get purged
					vertexCache.Touch( lightTris->ambientCache );

					// regenerate the lighting cache (for non-vertex program cards) if it has been purged
					if ( !lightTris->lightingCache ) {
						if ( !R_CreateLightingCache( entityDef, lightDef, lightTris ) ) {
							// skip if we are out of vertex memory
							continue;
						}
					}
					// touch the light surface so it won't get purged
					// (vertex program cards won't have a light cache at all)
					if ( lightTris->lightingCache ) {
						vertexCache.Touch( lightTris->lightingCache );
					}

					if ( !lightTris->indexCache && r_useIndexBuffers.GetBool() ) {
						vertexCache.Alloc( lightTris->indexes, lightTris->numIndexes * sizeof( lightTris->indexes[0] ), &lightTris->indexCache, true );
					}
					if ( lightTris->indexCache ) {
						vertexCache.Touch( lightTris->indexCache );
					}
				}
			}
		}
	}
}

/*
===================
R_ShowInteractionMemory_f
===================
*/
void R_ShowInteractionMemory_f( const idCmdArgs &args ) {
	int total = 0;
	int entities = 0;
	int interactions = 0;
	int deferredInteractions = 0;
	int emptyInteractions = 0;
	int lightTris = 0;
	int lightTriVerts = 0;
	int lightTriIndexes = 0;
	int shadowTris = 0;
	int shadowTriVerts = 0;
	int shadowTriIndexes = 0;

	for ( int i = 0; i < tr.primaryWorld->entityDefs.Num(); i++ ) {
		idRenderEntityLocal	*def = tr.primaryWorld->entityDefs[i];
		if ( !def ) {
			continue;
		}
		if ( def->firstInteraction == NULL ) {
			continue;
		}
		entities++;

		for ( idInteraction *inter = def->firstInteraction; inter != NULL; inter = inter->entityNext ) {
			interactions++;
			total += inter->MemoryUsed();

			if ( inter->IsDeferred() ) {
				deferredInteractions++;
				continue;
			}
			if ( inter->IsEmpty() ) {
				emptyInteractions++;
				continue;
			}

			for ( int j = 0; j < inter->numSurfaces; j++ ) {
				surfaceInteraction_t *srf = &inter->surfaces[j];

				if ( srf->ambientTris) {
					lightTris++;
					lightTriVerts += srf->ambientTris->numVerts;
					lightTriIndexes += srf->ambientTris->numIndexes;
				}
				//if ( srf->shadowTris ) {
				//	shadowTris++;
				//	shadowTriVerts += srf->shadowTris->numVerts;
				//	shadowTriIndexes += srf->shadowTris->numIndexes;
				//}
			}
		}
	}

	common->Printf( "%i entities with %i total interactions totalling %ik\n", entities, interactions, total / 1024 );
	common->Printf( "%i deferred interactions, %i empty interactions\n", deferredInteractions, emptyInteractions );
	common->Printf( "%5i indexes %5i verts in %5i light tris\n", lightTriIndexes, lightTriVerts, lightTris );
	common->Printf( "%5i indexes %5i verts in %5i shadow tris\n", shadowTriIndexes, shadowTriVerts, shadowTris );
}
