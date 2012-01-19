// Copyright (c) 2010-2011 Zipline Games, Inc. All Rights Reserved.
// http://getmoai.com

#include "pch.h"
#include <moaicore/MOAIBox2DWorld.h>
#include <moaicore/MOAICamera.h>
#include <moaicore/MOAICpSpace.h>
#include <moaicore/MOAIDebugLines.h>
#include <moaicore/MOAIDeck.h>
#include <moaicore/MOAIFrameBuffer.h>
#include <moaicore/MOAIGfxDevice.h>
#include <moaicore/MOAILayer.h>
#include <moaicore/MOAILogMessages.h>
#include <moaicore/MOAIPartitionResultBuffer.h>
#include <moaicore/MOAIPartitionResultMgr.h>
#include <moaicore/MOAIProp.h>
#include <moaicore/MOAITexture.h>
#include <moaicore/MOAITransform.h>

//================================================================//
// local
//================================================================//

//----------------------------------------------------------------//
/**	@name	clear
	@text	Remove all props from the layer's partition.
	
	@in		MOAILayer self
	@out	nil
*/
int MOAILayer::_clear ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "U" )

	if ( self->mPartition ) {
		self->mPartition->Clear ();
	}
	return 0;
}

//----------------------------------------------------------------//
/**	@name	getFitting
	@text	Computes a camera fitting for a given world rect along with
			an optional screen space padding. To do a fitting, computer
			the world rect based on whatever you are fitting to, use
			this method to get the fitting, then animate the camera
			to match.
	
	@in		MOAILayer self
	@in		number xMin
	@in		number yMin
	@in		number xMax
	@in		number yMax
	@opt	number xPad
	@opt	number yPad
	@out	number x		X center of fitting (use for camera location).
	@out	number y		Y center of fitting (use for camera location).
	@out	number s		Scale of fitting (use for camera scale).
*/
int MOAILayer::_getFitting ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UNNNN" )

	USRect worldRect;
	worldRect.mXMin = state.GetValue < float >( 2, 0.0f );
	worldRect.mYMin = state.GetValue < float >( 3, 0.0f );
	worldRect.mXMax = state.GetValue < float >( 4, 0.0f );
	worldRect.mYMax = state.GetValue < float >( 5, 0.0f );

	worldRect.Bless ();

	float hPad = state.GetValue < float >( 6, 0.0f );
	float vPad = state.GetValue < float >( 7, 0.0f );

	float x = worldRect.mXMin + (( worldRect.mXMax - worldRect.mXMin ) * 0.5f );
	float y = worldRect.mYMin + (( worldRect.mYMax - worldRect.mYMin ) * 0.5f );

	lua_pushnumber ( state, x );
	lua_pushnumber ( state, y );

	float fitting = self->GetFitting ( worldRect, hPad, vPad );
	lua_pushnumber ( state, fitting );

	return 3;
}

//----------------------------------------------------------------//
/**	@name	getPartition
	@text	Returns the partition (if any) currently attached to this layer.
	
	@in		MOAILayer self
	@out	MOAIPartition partition
*/
int	MOAILayer::_getPartition ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "U" )

	if ( self->mPartition ) {
		self->mPartition->PushLuaUserdata ( state );
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------//
/**	@name	getSortMode
	@text	Get the sort mode for rendering.
	
	@in		MOAILayer self
	@out	number sortMode
*/
int MOAILayer::_getSortMode ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "U" )
	
	lua_pushnumber ( state, self->mSortMode );
	return 1;
}

//----------------------------------------------------------------//
/**	@name	getSortScale
	@text	Return the scalar applied to axis sorts.
	
	@in		MOAILayer2D self
	@out	number x
	@out	number y
	@out	number priority
*/
int	MOAILayer::_getSortScale ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "U" )

	lua_pushnumber ( state, self->mSortScale [ 0 ]);
	lua_pushnumber ( state, self->mSortScale [ 1 ]);
	lua_pushnumber ( state, self->mSortScale [ 3 ]);

	return 3;
}

//----------------------------------------------------------------//
/**	@name	insertProp
	@text	Adds a prop to the layer's partition.
	
	@in		MOAILayer self
	@in		MOAIProp prop
	@out	nil
*/
int	MOAILayer::_insertProp ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UU" )

	MOAIProp* prop = state.GetLuaObject < MOAIProp >( 2 );
	if ( !prop ) return 0;
	if ( prop == self ) return 0;

	self->AffirmPartition ();
	self->mPartition->InsertProp ( *prop );
	prop->ScheduleUpdate ();

	return 0;
}

//----------------------------------------------------------------//
/**	@name	removeProp
	@text	Removes a prop from the layer's partition.
	
	@in		MOAILayer self
	@in		MOAIProp prop
	@out	nil
*/
int	MOAILayer::_removeProp ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UU" )

	MOAIProp* prop = state.GetLuaObject < MOAIProp >( 2 );
	if ( !prop ) return 0;
	if ( prop == self ) return 0;

	if ( self->mPartition ) {
		self->mPartition->RemoveProp ( *prop );
		prop->ScheduleUpdate ();
	}

	return 0;
}

//----------------------------------------------------------------//
/**	@name	setBox2DWorld
	@text	Sets a Box2D world for debug drawing.
	
	@in		MOAILayer self
	@in		MOAIBox2DWorld world
	@out	nil
*/
int MOAILayer::_setBox2DWorld ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UU" )
	
	#if USE_BOX2D
		self->mBox2DWorld.Set ( *self, state.GetLuaObject < MOAIBox2DWorld >( 2 ));
	#endif
	return 0;
}

//----------------------------------------------------------------//
/**	@name	setCamera
	@text	Sets a camera for the layer. If no camera is supplied,
			layer will render using the identity matrix as view/proj.
	
	@in		MOAILayer self
	@opt	MOAICamera camera	Default value is nil.
	@out	nil
*/
int MOAILayer::_setCamera ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "U" )

	self->mCamera.Set ( *self, state.GetLuaObject < MOAICamera >( 2 ));

	return 0;
}

//----------------------------------------------------------------//
/**	@name	setCpSpace
	@text	Sets a Chipmunk space for debug drawing.
	
	@in		MOAILayer self
	@in		MOAICpSpace space
	@out	nil
*/
int MOAILayer::_setCpSpace ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UU" )
	
	#if USE_CHIPMUNK
		self->mCpSpace.Set ( *self, state.GetLuaObject < MOAICpSpace >( 2 ));
	#endif
	return 0;
}

//----------------------------------------------------------------//
/**	@name	setFrameBuffer
	@text	Attach a frame buffer. Layer will render to frame buffer
			instead of the main view.
	
	@in		MOAILayer self
	@in		MOAITexture frameBuffer
	@out	nil
*/
int MOAILayer::_setFrameBuffer ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UU" )

	self->mFrameBuffer.Set ( *self, state.GetLuaObject < MOAITexture >( 2 ));

	return 0;
}

//----------------------------------------------------------------//
/**	@name	setParallax
	@text	Sets the parallax scale for this layer. This is simply a
			scalar applied to the view transform before rendering.
	
	@in		MOAILayer self
	@in		number xParallax
	@in		number yParallax
	@out	nil
*/
int MOAILayer::_setParallax ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UNN" )

	self->mParallax.mX = state.GetValue < float >( 2, self->mParallax.mX );
	self->mParallax.mY = state.GetValue < float >( 3, self->mParallax.mY );

	return 0;
}

//----------------------------------------------------------------//
/**	@name	setPartition
	@text	Sets a partition for the layer to use. The layer will automatically
			create a partition when the first prop is added if no partition
			has been set.
	
	@in		MOAILayer self
	@in		MOAIPartition partition
	@out	nil
*/
int MOAILayer::_setPartition ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UU" )

	self->mPartition.Set ( *self, state.GetLuaObject < MOAIPartition >( 2 ));

	return 0;
}

//----------------------------------------------------------------//
/**	@name	setPartitionCull2D
	@text	Enables 2D partition cull (projection of frustum AABB will
			be used instead of AABB or frustum).
	
	@in		MOAILayer self
	@in		boolean partitionCull2D		Default value is false.
	@out	nil
*/
int	MOAILayer::_setPartitionCull2D ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "U" )

	self->mPartitionCull2D = state.GetValue < bool >( 2, false );

	return 0;
}

//----------------------------------------------------------------//
/**	@name	setSortMode
	@text	Set the sort mode for rendering.
	
	@in		MOAILayer self
	@in		number sortMode		One of MOAILayer.SORT_NONE, MOAILayer.SORT_PRIORITY_ASCENDING,
								MOAILayer.SORT_PRIORITY_DESCENDING, MOAILayer.SORT_X_ASCENDING,
								MOAILayer.SORT_X_DESCENDING, MOAILayer.SORT_Y_ASCENDING,
								MOAILayer.SORT_Y_DESCENDING
	@out	nil
*/
int MOAILayer::_setSortMode ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "U" )
	
	self->mSortMode = state.GetValue < u32 >( 2, MOAIPartitionResultBuffer::SORT_PRIORITY_ASCENDING );
	
	return 0;
}

//----------------------------------------------------------------//
/**	@name	setSortScale
	@text	Set the scalar applied to axis sorts.
	
	@in		MOAILayer self
	@opt	number x			Default value is 0.
	@opt	number y			Default value is 0.
	@opt	number z			Default value is 0.
	@opt	number priority		Default value is 1.
	@out	nil
*/
int	MOAILayer::_setSortScale ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "U" )

	self->mSortScale [ 0 ] = state.GetValue < float >( 2, 0.0f );
	self->mSortScale [ 1 ] = state.GetValue < float >( 3, 0.0f );
	self->mSortScale [ 2 ] = state.GetValue < float >( 4, 0.0f );
	self->mSortScale [ 3 ] = state.GetValue < float >( 5, 1.0f );

	return 0;
}

//----------------------------------------------------------------//
/**	@name	setViewport
	@text	Set the layer's viewport.
	
	@in		MOAILayer self
	@in		MOAIViewport viewport
	@out	nil
*/
int MOAILayer::_setViewport ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UU" )

	self->mViewport.Set ( *self, state.GetLuaObject < MOAIViewport >( 2 ));

	return 0;
}

//----------------------------------------------------------------//
/**	@name	showDebugLines
	@text	Display debug lines for props in this layer.
	
	@in		MOAILayer self
	@opt	bool showDebugLines		Default value is 'true'.
	@out	nil
*/
int	MOAILayer::_showDebugLines ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "U" )
	
	self->mShowDebugLines = state.GetValue < bool >( 2, true );
	
	return 0;
}

//----------------------------------------------------------------//
/**	@name	wndToWorld
	@text	Project a point from window space into world space and return
			a normal vector representing a ray cast from the point into
			the world away from the camera (suitable for 3D picking).
	
	@in		MOAILayer self
	@in		number x
	@in		number y
	@in		number z
	@out	number x
	@out	number y
	@out	number z
	@out	number xn
	@out	number yn
	@out	number zn
*/
int MOAILayer::_wndToWorld ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UNN" )

	USVec4D loc;
	loc.mX = state.GetValue < float >( 2, 0.0f );
	loc.mY = state.GetValue < float >( 3, 0.0f );
	loc.mZ = state.GetValue < float >( 4, 0.0f );
	loc.mW = 1.0f;

	USVec4D vec = loc;
	vec.mZ += 0.1f;

	USMatrix4x4 wndToWorld;
	self->GetWndToWorldMtx ( wndToWorld );
	
	wndToWorld.Project ( loc );
	wndToWorld.Project ( vec );

	lua_pushnumber ( state, loc.mX );
	lua_pushnumber ( state, loc.mY );
	lua_pushnumber ( state, loc.mZ );

	USVec3D norm;

	norm.mX = vec.mX - loc.mX;
	norm.mY = vec.mY - loc.mY;
	norm.mZ = vec.mZ - loc.mZ;
	
	norm.Norm ();
	
	lua_pushnumber ( state, norm.mX );
	lua_pushnumber ( state, norm.mY );
	lua_pushnumber ( state, norm.mZ );

	return 6;
}

//----------------------------------------------------------------//
/**	@name	worldToWnd
	@text	Transform a point from world space to window space.
	
	@in		MOAILayer self
	@in		number x
	@in		number y
	@in		number Z
	@out	number x
	@out	number y
	@out	number z
*/
int MOAILayer::_worldToWnd ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAILayer, "UNN" )

	USVec4D loc;
	loc.mX = state.GetValue < float >( 2, 0.0f );
	loc.mY = state.GetValue < float >( 3, 0.0f );
	loc.mZ = state.GetValue < float >( 4, 0.0f );
	loc.mW = 1.0f;

	USMatrix4x4 worldToWnd;
	self->GetWorldToWndMtx ( worldToWnd );
	worldToWnd.Project ( loc );

	lua_pushnumber ( state, loc.mX );
	lua_pushnumber ( state, loc.mY );
	lua_pushnumber ( state, loc.mZ );

	return 3;
}

//================================================================//
// MOAINode
//================================================================//

//----------------------------------------------------------------//
void MOAILayer::AffirmPartition () {

	if ( !this->mPartition ) {
		this->mPartition.Set ( *this, new MOAIPartition ());
		
		MOAILuaStateHandle state = MOAILuaRuntime::Get ().State ();
		this->mPartition->PushLuaUserdata ( state );
		state.Pop ( 1 );
	}
}

//----------------------------------------------------------------//
void MOAILayer::Draw ( int subPrimID, bool reload ) {
	UNUSED ( subPrimID );
	UNUSED ( reload );

	if ( !this->mViewport ) return;
	
	MOAIViewport& viewport = *this->mViewport;
	MOAIGfxDevice& gfxDevice = MOAIGfxDevice::Get ();
	
	gfxDevice.ResetState ();
	gfxDevice.SetFrameBuffer ( this->mFrameBuffer );
	
	USRect viewportRect = viewport;

	// TODO:	
	if ( !this->IsOffscreen ()) {
		USMatrix4x4 mtx;
		mtx.Init ( this->mLocalToWorldMtx );
		// TODO:
		//mtx.Append ( gfxDevice.GetWorldToWndMtx ( 1.0f, 1.0f ));
		mtx.Transform ( viewportRect );
	}
	gfxDevice.SetViewport ( viewportRect );
	
	USMatrix4x4 view;
	this->GetViewMtx ( view );
	
	USMatrix4x4 proj;
	this->GetProjectionMtx ( proj );
	
	gfxDevice.SetVertexTransform ( MOAIGfxDevice::VTX_WORLD_TRANSFORM );
	gfxDevice.SetVertexTransform ( MOAIGfxDevice::VTX_VIEW_TRANSFORM, view );
	gfxDevice.SetVertexTransform ( MOAIGfxDevice::VTX_PROJ_TRANSFORM, proj );
	
	// recompute the frustum
	gfxDevice.UpdateViewVolume ();
	
	if ( this->mShowDebugLines ) {
		
		#if USE_CHIPMUNK
			if ( this->mCpSpace ) {
				this->mCpSpace->DrawDebug ();
				gfxDevice.Flush ();
			}
		#endif
		
		#if USE_BOX2D
			if ( this->mBox2DWorld ) {
				this->mBox2DWorld->DrawDebug ();
				gfxDevice.Flush ();
			}
		#endif
	}
	
	gfxDevice.SetVertexTransform ( MOAIGfxDevice::VTX_WORLD_TRANSFORM );
	gfxDevice.SetVertexTransform ( MOAIGfxDevice::VTX_VIEW_TRANSFORM, view );
	gfxDevice.SetVertexTransform ( MOAIGfxDevice::VTX_PROJ_TRANSFORM, proj );
	
	if ( this->mPartition ) {
		
		MOAIPartitionResultBuffer& buffer = MOAIPartitionResultMgr::Get ().GetBuffer ();
		const USFrustum& viewVolume = gfxDevice.GetViewVolume ();
		
		u32 totalResults = 0;
		
		if ( this->mPartitionCull2D ) {
			USRect rect = viewVolume.mAABB.GetRectXY ();
			totalResults = this->mPartition->GatherProps ( buffer, 0, rect, MOAIProp::CAN_DRAW | MOAIProp::CAN_DRAW_DEBUG );
		}
		else {
			totalResults = this->mPartition->GatherProps ( buffer, 0, viewVolume, MOAIProp::CAN_DRAW | MOAIProp::CAN_DRAW_DEBUG );
		}
		
		if ( !totalResults ) return;
		
		totalResults = buffer.PrepareResults (
			this->mSortMode,
			true,
			this->mSortScale [ 0 ],
			this->mSortScale [ 1 ],
			this->mSortScale [ 2 ],
			this->mSortScale [ 3 ]
		);

		MOAIProp* prevProp = 0;
		
		// render the sorted list
		for ( u32 i = 0; i < totalResults; ++i ) {
			MOAIPartitionResult* result = buffer.GetResultUnsafe ( i );
			
			MOAIProp* prop = result->mProp;
			bool reloadProp = prop != prevProp;
			
			prop->Draw ( result->mSubPrimID, reloadProp );
			prop->DrawDebug ( result->mSubPrimID );
			
			prevProp = prop;
		}
	}
	
	// render the debug lines
	if ( this->mShowDebugLines ) {
		MOAIDebugLines::Get ().Draw ();
	}
	gfxDevice.Flush ();
}

//----------------------------------------------------------------//
float MOAILayer::GetFitting ( USRect& worldRect, float hPad, float vPad ) {

	if ( !( this->mCamera && this->mViewport )) return 1.0f;

	USRect viewRect = this->mViewport->GetRect ();
	
	float hFit = ( viewRect.Width () - ( hPad * 2 )) / worldRect.Width ();
	float vFit = ( viewRect.Height () - ( vPad * 2 )) / worldRect.Height ();
	
	return ( hFit < vFit ) ? hFit : vFit;
}

//----------------------------------------------------------------//
u32 MOAILayer::GetDeckBounds ( USBox& bounds ) {
	
	if ( this->mViewport ) {
		USRect frame = this->mViewport->GetRect ();
		bounds.Init ( frame.mXMin, frame.mYMax, frame.mXMax, frame.mYMin, 0.0f, 0.0f );
		return MOAIProp::BOUNDS_OK;
	}
	return MOAIProp::BOUNDS_EMPTY;
}

//----------------------------------------------------------------//
void MOAILayer::GetProjectionMtx ( USMatrix4x4& proj ) {
	
	if ( this->mCamera ) {
		proj.Init ( this->mCamera->GetProjMtx ( *this->mViewport ));
	}
	else {
		proj.Init ( this->mViewport->GetProjMtx ());
	}
}

//----------------------------------------------------------------//
void MOAILayer::GetViewMtx ( USMatrix4x4& view ) {
	
	if ( this->mCamera ) {	
		view.Init ( this->mCamera->GetViewMtx ());
	}
	else {
		view.Ident ();
	}
}

//----------------------------------------------------------------//
void MOAILayer::GetWndToWorldMtx ( USMatrix4x4& wndToWorld ) {

	if ( this->mViewport ) {
		
		this->GetWorldToWndMtx ( wndToWorld );
		wndToWorld.Inverse ();
	}
	else {
		wndToWorld.Ident ();
	}
}

//----------------------------------------------------------------//
void MOAILayer::GetWorldToWndMtx ( USMatrix4x4& worldToWnd ) {

	if ( this->mViewport ) {
		
		USMatrix4x4 view;
		this->GetViewMtx ( view );

		USMatrix4x4 proj;
		this->GetProjectionMtx ( proj );
		
		USMatrix4x4 wnd;
		this->mViewport->GetNormToWndMtx ( wnd );
		
		worldToWnd = view;
		worldToWnd.Append ( proj );
		worldToWnd.Append ( wnd );
	}
	else {
		worldToWnd.Ident ();
	}
}

//----------------------------------------------------------------//
bool MOAILayer::IsOffscreen () {

	return this->mFrameBuffer ? this->mFrameBuffer->GetFrameBuffer () != 0 : false;
}

//----------------------------------------------------------------//
MOAILayer::MOAILayer () :
	mParallax ( 1.0f, 1.0f ),
	mShowDebugLines ( true ),
	mSortMode ( MOAIPartitionResultBuffer::SORT_PRIORITY_ASCENDING ),
	mPartitionCull2D ( false ) {
	
	RTTI_BEGIN
		RTTI_EXTEND ( MOAIProp )
	RTTI_END
	
	this->SetMask ( MOAIProp::CAN_DRAW | MOAIProp::CAN_DRAW_DEBUG );
}

//----------------------------------------------------------------//
MOAILayer::~MOAILayer () {

	this->mCamera.Set ( *this, 0 );
	this->mViewport.Set ( *this, 0 );
	this->mPartition.Set ( *this, 0 );
	this->mFrameBuffer.Set ( *this, 0 );

	#if USE_CHIPMUNK
		this->mCpSpace.Set ( *this, 0 );
	#endif
	
	#if USE_BOX2D
		this->mBox2DWorld.Set ( *this, 0 );
	#endif
}

//----------------------------------------------------------------//
void MOAILayer::RegisterLuaClass ( MOAILuaState& state ) {

	MOAIProp::RegisterLuaClass ( state );
	
	state.SetField ( -1, "SORT_NONE",					( u32 ) MOAIPartitionResultBuffer::SORT_NONE );
	state.SetField ( -1, "SORT_PRIORITY_ASCENDING",		( u32 ) MOAIPartitionResultBuffer::SORT_PRIORITY_ASCENDING );
	state.SetField ( -1, "SORT_PRIORITY_DESCENDING",	( u32 ) MOAIPartitionResultBuffer::SORT_PRIORITY_DESCENDING );
	state.SetField ( -1, "SORT_X_ASCENDING",			( u32 ) MOAIPartitionResultBuffer::SORT_X_ASCENDING );
	state.SetField ( -1, "SORT_X_DESCENDING",			( u32 ) MOAIPartitionResultBuffer::SORT_X_DESCENDING );
	state.SetField ( -1, "SORT_Y_ASCENDING",			( u32 ) MOAIPartitionResultBuffer::SORT_Y_ASCENDING );
	state.SetField ( -1, "SORT_Y_DESCENDING",			( u32 ) MOAIPartitionResultBuffer::SORT_Y_DESCENDING );
	state.SetField ( -1, "SORT_VECTOR_ASCENDING",		( u32 ) MOAIPartitionResultBuffer::SORT_VECTOR_ASCENDING );
	state.SetField ( -1, "SORT_VECTOR_DESCENDING",		( u32 ) MOAIPartitionResultBuffer::SORT_VECTOR_DESCENDING );
}

//----------------------------------------------------------------//
void MOAILayer::RegisterLuaFuncs ( MOAILuaState& state ) {
	
	MOAIProp::RegisterLuaFuncs ( state );
	
	luaL_Reg regTable [] = {
		{ "clear",					_clear },
		{ "getFitting",				_getFitting },
		{ "getPartition",			_getPartition },
		{ "getSortMode",			_getSortMode },
		{ "getSortScale",			_getSortScale },
		{ "insertProp",				_insertProp },
		{ "removeProp",				_removeProp },
		{ "setBox2DWorld",			_setBox2DWorld },
		{ "setCamera",				_setCamera },
		{ "setCpSpace",				_setCpSpace },
		{ "setFrameBuffer",			_setFrameBuffer },
		{ "setParallax",			_setParallax },
		{ "setPartition",			_setPartition },
		{ "setPartitionCull2D",		_setPartitionCull2D },
		{ "setSortMode",			_setSortMode },
		{ "setSortScale",			_setSortScale },
		{ "setViewport",			_setViewport },
		{ "showDebugLines",			_showDebugLines },
		{ "wndToWorld",				_wndToWorld },
		{ "worldToWnd",				_worldToWnd },
		{ NULL, NULL }
	};
	
	luaL_register ( state, 0, regTable );
}