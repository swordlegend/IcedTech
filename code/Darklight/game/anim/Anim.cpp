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

#include "game_precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

bool idAnimManager::forceExport = false;

static const byte B_ANIM_MD5_VERSION = 101;
static const unsigned int B_ANIM_MD5_MAGIC = ('B' << 24) | ('M' << 16) | ('D' << 8) | B_ANIM_MD5_VERSION;

/***********************************************************************

	idMD5Anim

***********************************************************************/

/*
====================
idMD5Anim::idMD5Anim
====================
*/
idMD5Anim::idMD5Anim() {
	ref_count	= 0;
	numFrames	= 0;
	numJoints	= 0;
	frameRate	= 24;
	animLength	= 0;
// jmarshall - md6 anim
	animType = ANIM_TYPE_UNKNOWN;
// jmarshall end
	totaldelta.Zero();
}

/*
====================
idMD5Anim::idMD5Anim
====================
*/
idMD5Anim::~idMD5Anim() {
	Free();
}

/*
====================
idMD5Anim::Free
====================
*/
void idMD5Anim::Free( void ) {
	numFrames	= 0;
	numJoints	= 0;
	frameRate	= 24;
	animLength	= 0;
	name		= "";

	totaldelta.Zero();

	jointInfo.Clear();
	bounds.Clear();
	componentFrames.Clear();
}

/*
====================
idMD5Anim::NumFrames
====================
*/
int	idMD5Anim::NumFrames( void ) const {
	return numFrames;
}

/*
====================
idMD5Anim::NumJoints
====================
*/
int	idMD5Anim::NumJoints( void ) const {
	return numJoints;
}

/*
====================
idMD5Anim::Length
====================
*/
int idMD5Anim::Length( void ) const {
	return animLength;
}

/*
=====================
idMD5Anim::TotalMovementDelta
=====================
*/
const idVec3 &idMD5Anim::TotalMovementDelta( void ) const {
	return totaldelta;
}

/*
=====================
idMD5Anim::TotalMovementDelta
=====================
*/
const char *idMD5Anim::Name( void ) const {
	return name;
}

/*
====================
idMD5Anim::Reload
====================
*/
bool idMD5Anim::Reload( void ) {
	idStr filename;

	filename = name;
	Free();

	return LoadAnim( filename );
}

/*
====================
idMD5Anim::Allocated
====================
*/
size_t idMD5Anim::Allocated( void ) const {
	size_t	size = bounds.Allocated() + jointInfo.Allocated() + componentFrames.Allocated() + name.Allocated();
	return size;
}

// jmarshall - md6 anim support.
/*
====================
idMD5Anim::ParseMD6Flags
====================
*/
bool idMD5Anim::ParseMD6Flags(idLexer& parser) {
	idToken token;

	parser.ExpectTokenString("flags");
	parser.ExpectTokenString("{");

	while (true) {
		if (parser.EndOfFile()) {
			parser.Error("Unexpceted EOF in flags block\n");
			return false;
		}

		parser.ReadToken(&token);

		if (token == "}") {
			break;
		}
		else if (token == "additive") {
			// TODO Implement me!
		}
		else if (token == "useForwardTranslation") {
			// TODO Implement me!
		}
		else if (token == "useLeftTranslation") {
			// TODO Implement me!
		}
		else if (token == "useUpTranslation") {
			// TODO Implement me!
		}
		else if (token == "useYawRotation" || token == "useRotation") {
			// TODO Implement me!
		}
		else if (token == "ignoreBounds") {
			// TODO Implement me!
		}
		else if (token == "retargetAdditive") {
			// TODO Implement me!
		}
		else {
			parser.Error("Unexpected or unknown token in flags block %s\n", token.c_str());
		}
	}
	return true;
}

/*
====================
idMD5Anim::ParseMD6Init
====================
*/
bool idMD5Anim::ParseMD6Init(idLexer& parser) {
	idToken token;

	parser.ExpectTokenString("init");
	parser.ExpectTokenString("{");

	while(true) {
		if(parser.EndOfFile()) {
			parser.Error("Unexpceted EOF in init block\n");
			return false;
		}

		parser.ReadToken(&token);

		if(token == "}") {
			break;
		}
		else if(token == "commandLine") {
			parser.ReadToken(&token);
		}
		else if (token == "sourceAnim") {
			parser.ReadToken(&token);
		}
		else if (token == "subtractiveAnim") { // TODO implement this
			parser.ReadToken(&token);
		}
		else if (token == "rotationMask") { // TODO implement this
			parser.ReadToken(&token);
		}
		else if (token == "scaleMask") { // TODO implement this
			parser.ReadToken(&token);
		}
		else if (token == "translationMask") { // TODO implement this
			parser.ReadToken(&token);
		}
		else if (token == "skeletonName") { // Are MD6skel used at runtime?
			parser.ReadToken(&token);
		}
		else if (token == "meshName") { // Do we need to keep track of the md6mesh at runtime?
			parser.ReadToken(&token);
			renderModel = renderModelManager->FindModel(token);
			if (renderModel == NULL) {
				parser.Error("Failed to load render model %s\n", token.c_str());
			}
		}
		else if (token == "numFrames") { 
			numFrames = parser.ParseInt();
		}
		else if (token == "frameRate") {
			frameRate = parser.ParseInt();
		}
		else if (token == "numJoints") {
			numJoints = parser.ParseInt();
		}
		else if (token == "numUserChannels") {  // TODO implement this
			parser.ParseInt();
		}
		else if(token == "translatedBounds") {
			idBounds _bounds;

			parser.Parse1DMatrix(3, _bounds[0].ToFloatPtr());
			parser.Parse1DMatrix(3, _bounds[1].ToFloatPtr());

			bounds.SetGranularity(1);
			bounds.SetNum(numFrames);
			for (int i = 0; i < numFrames; i++) {
				bounds[i] = _bounds;
			}
		}
		else if (token == "normalizedBounds") {
			idBounds bounds;

			parser.Parse1DMatrix(3, bounds[0].ToFloatPtr());
			parser.Parse1DMatrix(3, bounds[1].ToFloatPtr());
		}
		else if (token == "maxErrorRotation") {
			parser.ParseFloat();
		}
		else if (token == "maxErrorScale") {
			parser.ParseFloat();
		}
		else if (token == "maxErrorTranslation") {
			parser.ParseFloat();
		}
		else if (token == "maxErrorUser") {
			parser.ParseFloat();
		}
		else {
			parser.Error("Unexpected or unknown token in init block %s\n", token.c_str());
		}
	}
	
	return true;
}

/*
====================
idMD5Anim::LoadMD6Anim
====================
*/
bool idMD5Anim::LoadMD6Anim(const char* filename) {
	int		version;
	idLexer	parser(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT);
	idToken	token;
	int		i, j;
	int		num;

// jmarshall
	animType = ANIM_TYPE_MD6;
// jmarshall end

	idStr generatedFileName = "generated/anim/";
	generatedFileName.AppendPath(filename);
	generatedFileName.SetFileExtension(".bMD6anim");

	// Get the timestamp on the original file, if it's newer than what is stored in binary model, regenerate it
	ID_TIME_T sourceTimeStamp = fileSystem->GetTimestamp(filename);

	//idFileScoped file(fileSystem->OpenFileReadMemory(generatedFileName));
	//if (LoadBinary(file, sourceTimeStamp)) {
	//	name = filename;
	//	return true;
	//}

	if (!parser.LoadFile(filename)) {
		return false;
	}

	Free();

	name = filename;

	parser.ExpectTokenString(MD6_VERSION_STRING);
	version = parser.ParseInt();
	if (version != MD6_ANIM_VERSION) {
		parser.Error("Invalid version %d.  Should be version %d\n", version, MD6_ANIM_VERSION);
	}

	// Parse the init block
	ParseMD6Init(parser);

	// Parse the flags block
	ParseMD6Flags(parser);

	// parse the hierarchy
	jointInfo.SetGranularity(1);
	jointInfo.SetNum(numJoints);
	parser.ExpectTokenString("joints");
	parser.ExpectTokenString("{");
	for (i = 0; i < numJoints; i++) {
		parser.ReadToken(&token);
		jointInfo[i].nameIndex = animationLib.JointIndex(token);

		// parse parent num
		jointInfo[i].parentNum = parser.ParseInt();
		if (jointInfo[i].parentNum >= i) {
			parser.Error("Invalid parent num: %d", jointInfo[i].parentNum);
		}

		if ((i != 0) && (jointInfo[i].parentNum < 0)) {
			parser.Error("Animations may have only one root joint");
		}

		// jmarshall: TODO It seems to be always 1 that follows the parent num but I don't know what it is.
		int unknownToken = parser.ParseInt();
		if(unknownToken != 1) {
			common->FatalError("MD6 Anim has unknowntoken of non 1, needs to be looked at!\n");
		}

		// Disable a bunch of the old animation system.
		jointInfo[i].animBits = -1; // (ANIM_TX | ANIM_TY | ANIM_TZ | ANIM_QX | ANIM_QY | ANIM_QZ);
		jointInfo[i].firstComponent = -1;		
	}

	parser.ExpectTokenString("}");

	// TODO Implement rotation/scale/translation masks.
	idStr temp;
	parser.ExpectTokenString("rotationMask");	
	parser.ParseBracedSectionExact(temp);

	parser.ExpectTokenString("scaleMask");
	parser.ParseBracedSectionExact(temp);

	parser.ExpectTokenString("translationMask");
	parser.ParseBracedSectionExact(temp);

	// Now grab all of our frames
	md6Frames.SetNum(numFrames);
	parser.ExpectTokenString("frames");
	parser.ExpectTokenString("{");
	for(int i = 0; i < numFrames; i++) {
		md6Frame_t* frame = &md6Frames[i];

		frame->jointFrames.SetNum(numJoints);

		parser.ExpectTokenString("frame");
		parser.ExpectTokenString(va("%d", i));
		parser.ExpectTokenString("{");
		for(int d = 0; d < numJoints; d++) {
			md6JointFrame_t* joint = &frame->jointFrames[d];

			joint->parentNum = jointInfo[d].parentNum;

			parser.ExpectTokenString("joint");
			parser.ExpectTokenString(va("%d", d));
			parser.ExpectTokenString("{");

			parser.ExpectTokenString("R");
			parser.Parse1DMatrix(4, joint->rotation.ToFloatPtr());

			parser.ExpectTokenString("S");
			parser.Parse1DMatrix(3, joint->scale.ToFloatPtr());

			parser.ExpectTokenString("T");
			parser.Parse1DMatrix(3, joint->translation.ToFloatPtr());

			parser.ExpectTokenString("}");
		}

		parser.ExpectTokenString("}");
	}
	parser.ExpectTokenString("}");

	// we don't count last frame because it would cause a 1 frame pause at the end
	animLength = ((numFrames - 1) * 1000 + frameRate - 1) / frameRate;

	common->Printf("Writing %s\n", generatedFileName.c_str());
	idFileScoped outputFile(fileSystem->OpenFileWrite(generatedFileName, "fs_basepath"));
	WriteBinary(outputFile, sourceTimeStamp);

	return true;
}
// jmarshall end

/*
====================
idMD5Anim::LoadAnim
====================
*/
bool idMD5Anim::LoadAnim( const char *filename ) {
	int		version;
	idLexer	parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	idToken	token;
	int		i, j;
	int		num;

// jmarshall
	animType = ANIM_TYPE_MD5;
// jmarshall end

	idStr generatedFileName = "generated/anim/";
	generatedFileName.AppendPath(filename);
	generatedFileName.SetFileExtension(".bMD5anim");

	// Get the timestamp on the original file, if it's newer than what is stored in binary model, regenerate it
	ID_TIME_T sourceTimeStamp = fileSystem->GetTimestamp(filename);

	idFileScoped file(fileSystem->OpenFileReadMemory(generatedFileName));
	if (LoadBinary(file, sourceTimeStamp)) {
		name = filename;
		return true;
	}

	if ( !parser.LoadFile( filename ) ) {
		return false;
	}

	Free();

	name = filename;

	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();
	if ( version != MD5_VERSION ) {
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION );
	}

	// skip the commandline
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num frames
	parser.ExpectTokenString( "numFrames" );
	numFrames = parser.ParseInt();
	if ( numFrames <= 0 ) {
		parser.Error( "Invalid number of frames: %d", numFrames );
	}

	// parse num joints
	parser.ExpectTokenString( "numJoints" );
	numJoints = parser.ParseInt();
	if ( numJoints <= 0 ) {
		parser.Error( "Invalid number of joints: %d", numJoints );
	}

	// parse frame rate
	parser.ExpectTokenString( "frameRate" );
	frameRate = parser.ParseInt();
	if ( frameRate < 0 ) {
		parser.Error( "Invalid frame rate: %d", frameRate );
	}

	// parse number of animated components
	parser.ExpectTokenString( "numAnimatedComponents" );
	numAnimatedComponents = parser.ParseInt();
	if ( ( numAnimatedComponents < 0 ) || ( numAnimatedComponents > numJoints * 6 ) ) {
		parser.Error( "Invalid number of animated components: %d", numAnimatedComponents );
	}

	// parse the hierarchy
	jointInfo.SetGranularity( 1 );
	jointInfo.SetNum( numJoints );
	parser.ExpectTokenString( "hierarchy" );
	parser.ExpectTokenString( "{" );
	for( i = 0; i < numJoints; i++ ) {
		parser.ReadToken( &token );
		jointInfo[ i ].nameIndex = animationLib.JointIndex( token );
		
		// parse parent num
		jointInfo[ i ].parentNum = parser.ParseInt();
		if ( jointInfo[ i ].parentNum >= i ) {
			parser.Error( "Invalid parent num: %d", jointInfo[ i ].parentNum );
		}

		if ( ( i != 0 ) && ( jointInfo[ i ].parentNum < 0 ) ) {
			parser.Error( "Animations may have only one root joint" );
		}

		// parse anim bits
		jointInfo[ i ].animBits = parser.ParseInt();
		if ( jointInfo[ i ].animBits & ~63 ) {
			parser.Error( "Invalid anim bits: %d", jointInfo[ i ].animBits );
		}

		// parse first component
		jointInfo[ i ].firstComponent = parser.ParseInt();
		if ( ( numAnimatedComponents > 0 ) && ( ( jointInfo[ i ].firstComponent < 0 ) || ( jointInfo[ i ].firstComponent >= numAnimatedComponents ) ) ) {
			parser.Error( "Invalid first component: %d", jointInfo[ i ].firstComponent );
		}
	}

	parser.ExpectTokenString( "}" );

	// parse bounds
	parser.ExpectTokenString( "bounds" );
	parser.ExpectTokenString( "{" );
	bounds.SetGranularity( 1 );
	bounds.SetNum( numFrames );
	for( i = 0; i < numFrames; i++ ) {
		parser.Parse1DMatrix( 3, bounds[ i ][ 0 ].ToFloatPtr() );
		parser.Parse1DMatrix( 3, bounds[ i ][ 1 ].ToFloatPtr() );
	}
	parser.ExpectTokenString( "}" );

	// parse base frame
	baseFrame.SetGranularity( 1 );
	baseFrame.SetNum( numJoints );
	parser.ExpectTokenString( "baseframe" );
	parser.ExpectTokenString( "{" );
	for( i = 0; i < numJoints; i++ ) {
		idCQuat q;
		parser.Parse1DMatrix( 3, baseFrame[ i ].t.ToFloatPtr() );
		parser.Parse1DMatrix( 3, q.ToFloatPtr() );//baseFrame[ i ].q.ToFloatPtr() );
		baseFrame[ i ].q = q.ToQuat();//.w = baseFrame[ i ].q.CalcW();
	}
	parser.ExpectTokenString( "}" );

	// parse frames
	componentFrames.SetGranularity( 1 );
	componentFrames.SetNum( numAnimatedComponents * numFrames );

	float *componentPtr = componentFrames.Ptr();
	for( i = 0; i < numFrames; i++ ) {
		parser.ExpectTokenString( "frame" );
		num = parser.ParseInt();
		if ( num != i ) {
			parser.Error( "Expected frame number %d", i );
		}
		parser.ExpectTokenString( "{" );
		
		for( j = 0; j < numAnimatedComponents; j++, componentPtr++ ) {
			*componentPtr = parser.ParseFloat();
		}

		parser.ExpectTokenString( "}" );
	}

	// get total move delta
	if ( !numAnimatedComponents ) {
		totaldelta.Zero();
	} else {
		componentPtr = &componentFrames[ jointInfo[ 0 ].firstComponent ];
		if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
			for( i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t.x;
			}
			totaldelta.x = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		} else {
			totaldelta.x = 0.0f;
		}
		if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
			for( i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t.y;
			}
			totaldelta.y = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		} else {
			totaldelta.y = 0.0f;
		}
		if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
			for( i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t.z;
			}
			totaldelta.z = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
		} else {
			totaldelta.z = 0.0f;
		}
	}
	baseFrame[ 0 ].t.Zero();

	// we don't count last frame because it would cause a 1 frame pause at the end
	animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;

	common->Printf("Writing %s\n", generatedFileName.c_str());
	idFileScoped outputFile(fileSystem->OpenFileWrite(generatedFileName, "fs_basepath"));
	WriteBinary(outputFile, sourceTimeStamp);

	// done
	return true;
}


/*
========================
idMD5Anim::LoadBinary
========================
*/
bool idMD5Anim::LoadBinary(idFile * file, ID_TIME_T sourceTimeStamp) {

	if (file == NULL) {
		return false;
	}

	unsigned int magic = 0;
	file->ReadBig(magic);
	if (magic != B_ANIM_MD5_MAGIC) {
		return false;
	}

	ID_TIME_T loadedTimeStamp;
	file->ReadBig(loadedTimeStamp);
	if (!common->InProductionMode() && sourceTimeStamp != loadedTimeStamp) {
		return false;
	}

	file->ReadBig(numFrames);
	file->ReadBig(frameRate);
	file->ReadBig(animLength);
	file->ReadBig(numJoints);
	file->ReadBig(numAnimatedComponents);

	int num;
	file->ReadBig(num);
	bounds.SetNum(num);
	for (int i = 0; i < num; i++) {
		idBounds & b = bounds[i];
		file->ReadBig(b[0]);
		file->ReadBig(b[1]);
	}

	file->ReadBig(num);
	jointInfo.SetNum(num);
	for (int i = 0; i < num; i++) {
		jointAnimInfo_t & j = jointInfo[i];

		idStr jointName;
		file->ReadString(jointName);
		if (jointName.IsEmpty()) {
			j.nameIndex = -1;
		}
		else {
			j.nameIndex = animationLib.JointIndex(jointName.c_str());
		}

		file->ReadBig(j.parentNum);
		file->ReadBig(j.animBits);
		file->ReadBig(j.firstComponent);
	}

	file->ReadBig(num);
	baseFrame.SetNum(num);
	for (int i = 0; i < num; i++) {
		idJointQuat & j = baseFrame[i];
		file->ReadBig(j.q.x);
		file->ReadBig(j.q.y);
		file->ReadBig(j.q.z);
		file->ReadBig(j.q.w);
		file->ReadVec3(j.t);
//		j.w = 0.0f;
	}

	file->ReadBig(num);
	componentFrames.SetNum(num);
	for (int i = 0; i < componentFrames.Num(); i++) {
		file->ReadFloat(componentFrames[i]);
	}

	//file->ReadString( name );
	file->ReadVec3(totaldelta);
	//file->ReadBig( ref_count );

	return true;
}

/*
========================
idMD5Anim::WriteBinary
========================
*/
void idMD5Anim::WriteBinary(idFile * file, ID_TIME_T sourceTimeStamp) {

	if (file == NULL) {
		return;
	}

	file->WriteBig(B_ANIM_MD5_MAGIC);
	file->WriteBig(sourceTimeStamp);

	file->WriteBig(numFrames);
	file->WriteBig(frameRate);
	file->WriteBig(animLength);
	file->WriteBig(numJoints);
	file->WriteBig(numAnimatedComponents);

	file->WriteBig(bounds.Num());
	for (int i = 0; i < bounds.Num(); i++) {
		idBounds & b = bounds[i];
		file->WriteBig(b[0]);
		file->WriteBig(b[1]);
	}

	file->WriteBig(jointInfo.Num());
	for (int i = 0; i < jointInfo.Num(); i++) {
		jointAnimInfo_t & j = jointInfo[i];
		idStr jointName = animationLib.JointName(j.nameIndex);
		file->WriteString(jointName);
		file->WriteBig(j.parentNum);
		file->WriteBig(j.animBits);
		file->WriteBig(j.firstComponent);
	}

	file->WriteBig(baseFrame.Num());
	for (int i = 0; i < baseFrame.Num(); i++) {
		idJointQuat & j = baseFrame[i];
		file->WriteBig(j.q.x);
		file->WriteBig(j.q.y);
		file->WriteBig(j.q.z);
		file->WriteBig(j.q.w);
		file->WriteVec3(j.t);
	}

	file->WriteBig(componentFrames.Num());
	for (int i = 0; i < componentFrames.Num(); i++) {
		file->WriteFloat(componentFrames[i]);
	}

	//file->WriteString( name );
	file->WriteVec3(totaldelta);
	//file->WriteBig( ref_count );
}


/*
====================
idMD5Anim::IncreaseRefs
====================
*/
void idMD5Anim::IncreaseRefs( void ) const {
	ref_count++;
}

/*
====================
idMD5Anim::DecreaseRefs
====================
*/
void idMD5Anim::DecreaseRefs( void ) const {
	ref_count--;
}

/*
====================
idMD5Anim::NumRefs
====================
*/
int idMD5Anim::NumRefs( void ) const {
	return ref_count;
}

/*
====================
idMD5Anim::GetFrameBlend
====================
*/
void idMD5Anim::GetFrameBlend( int framenum, frameBlend_t &frame ) const {
	frame.cycleCount	= 0;
	frame.backlerp		= 0.0f;
	frame.frontlerp		= 1.0f;

	// frame 1 is first frame
	framenum--;
	if ( framenum < 0 ) {
		framenum = 0;
	} else if ( framenum >= numFrames ) {
		framenum = numFrames - 1;
	}

	frame.frame1 = framenum;
	frame.frame2 = framenum;
}

/*
====================
idMD5Anim::ConvertTimeToFrame
====================
*/
void idMD5Anim::ConvertTimeToFrame( int time, int cyclecount, frameBlend_t &frame ) const {
	int frameTime;
	int frameNum;

	if ( numFrames <= 1 ) {
		frame.frame1		= 0;
		frame.frame2		= 0;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		frame.cycleCount	= 0;
		return;
	}

	if ( time <= 0 ) {
		frame.frame1		= 0;
		frame.frame2		= 1;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		frame.cycleCount	= 0;
		return;
	}
	
	frameTime			= time * frameRate;
	frameNum			= frameTime / 1000;
	frame.cycleCount	= frameNum / ( numFrames - 1 );

	if ( ( cyclecount > 0 ) && ( frame.cycleCount >= cyclecount ) ) {
		frame.cycleCount	= cyclecount - 1;
		frame.frame1		= numFrames - 1;
		frame.frame2		= frame.frame1;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		return;
	}
	
	frame.frame1 = frameNum % ( numFrames - 1 );
	frame.frame2 = frame.frame1 + 1;
	if ( frame.frame2 >= numFrames ) {
		frame.frame2 = 0;
	}

	frame.backlerp	= ( frameTime % 1000 ) * 0.001f;
	frame.frontlerp	= 1.0f - frame.backlerp;
}

/*
====================
idMD5Anim::GetOrigin
====================
*/
void idMD5Anim::GetOrigin( idVec3 &offset, int time, int cyclecount ) const {
	frameBlend_t frame;

// jmarshall
	if (animType == ANIM_TYPE_MD6) {
		idVec3 t;

		ConvertTimeToFrame(time, cyclecount, frame);

		t.Lerp(md6Frames[frame.frame1].jointFrames[0].translation, md6Frames[frame.frame2].jointFrames[0].translation, frame.backlerp);
		offset = t;
		return;
	}
// jmarshall end

	offset = baseFrame[ 0 ].t;
	if ( !( jointInfo[ 0 ].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) ) ) {
		// just use the baseframe		
		return;
	}

	ConvertTimeToFrame( time, cyclecount, frame );

	const float *componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
	const float *componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

	if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
		offset.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
		offset.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
		offset.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
	}

	if ( frame.cycleCount ) {
		offset += totaldelta * ( float )frame.cycleCount;
	}
}

/*
====================
idMD5Anim::GetOriginRotation
====================
*/
void idMD5Anim::GetOriginRotation( idQuat &rotation, int time, int cyclecount ) const {
	frameBlend_t	frame;
	int				animBits;

// jmarshall
	if (animType == ANIM_TYPE_MD6) {		
		ConvertTimeToFrame(time, cyclecount, frame);
		rotation.Slerp(md6Frames[frame.frame1].jointFrames[0].rotation, md6Frames[frame.frame2].jointFrames[0].rotation, frame.backlerp);
		return;
	}
// jmarshall end

	
	animBits = jointInfo[ 0 ].animBits;
	if ( !( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) ) ) {
		// just use the baseframe		
		rotation = baseFrame[ 0 ].q;
		return;
	}

	ConvertTimeToFrame( time, cyclecount, frame );

	const float	*jointframe1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
	const float	*jointframe2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

	if ( animBits & ANIM_TX ) {
		jointframe1++;
		jointframe2++;
	}

	if ( animBits & ANIM_TY ) {
		jointframe1++;
		jointframe2++;
	}

	if ( animBits & ANIM_TZ ) {
		jointframe1++;
		jointframe2++;
	}

	idQuat q1;
	idQuat q2;

	switch( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
		case ANIM_QX:
			q1.x = jointframe1[0];
			q2.x = jointframe2[0];
			q1.y = baseFrame[ 0 ].q.y;
			q2.y = q1.y;
			q1.z = baseFrame[ 0 ].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY:
			q1.y = jointframe1[0];
			q2.y = jointframe2[0];
			q1.x = baseFrame[ 0 ].q.x;
			q2.x = q1.x;
			q1.z = baseFrame[ 0 ].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QZ:
			q1.z = jointframe1[0];
			q2.z = jointframe2[0];
			q1.x = baseFrame[ 0 ].q.x;
			q2.x = q1.x;
			q1.y = baseFrame[ 0 ].q.y;
			q2.y = q1.y;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QY:
			q1.x = jointframe1[0];
			q1.y = jointframe1[1];
			q2.x = jointframe2[0];
			q2.y = jointframe2[1];
			q1.z = baseFrame[ 0 ].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QZ:
			q1.x = jointframe1[0];
			q1.z = jointframe1[1];
			q2.x = jointframe2[0];
			q2.z = jointframe2[1];
			q1.y = baseFrame[ 0 ].q.y;
			q2.y = q1.y;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY|ANIM_QZ:
			q1.y = jointframe1[0];
			q1.z = jointframe1[1];
			q2.y = jointframe2[0];
			q2.z = jointframe2[1];
			q1.x = baseFrame[ 0 ].q.x;
			q2.x = q1.x;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QY|ANIM_QZ:
			q1.x = jointframe1[0];
			q1.y = jointframe1[1];
			q1.z = jointframe1[2];
			q2.x = jointframe2[0];
			q2.y = jointframe2[1];
			q2.z = jointframe2[2];
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
	}

	rotation.Slerp( q1, q2, frame.backlerp );
}

/*
====================
idMD5Anim::GetBounds
====================
*/
void idMD5Anim::GetBounds( idBounds &bnds, int time, int cyclecount ) const {
	frameBlend_t	frame;
	idVec3			offset;

// jmarshall
	// We should evaluate this for non root animations!
	if(animType == ANIM_TYPE_MD6) {
		bnds = renderModel->Bounds();
		return;
	}
// jmarshall end

	ConvertTimeToFrame( time, cyclecount, frame );

	bnds = bounds[ frame.frame1 ];
	bnds.AddBounds( bounds[ frame.frame2 ] );

	// origin position
	offset = baseFrame[ 0 ].t;
	if ( jointInfo[ 0 ].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) ) {
		const float *componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
		const float *componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

		if ( jointInfo[ 0 ].animBits & ANIM_TX ) {
			offset.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if ( jointInfo[ 0 ].animBits & ANIM_TY ) {
			offset.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if ( jointInfo[ 0 ].animBits & ANIM_TZ ) {
			offset.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		}
	}

	bnds[ 0 ] -= offset;
	bnds[ 1 ] -= offset;
}

/*
====================
idMD5Anim::GetInterpolatedMD6Frame
====================
*/
void idMD5Anim::GetInterpolatedMD6Frame(frameBlend_t& frame, idJointQuat* joints, const int* index, int numIndexes) const {
	idJointQuat* blendJoints;
	int* lerpIndex;

	blendJoints = (idJointQuat*)_alloca16(md6Frames[0].jointFrames.Num() * sizeof(idJointQuat));

	// This looks silly because we are trying to reuse as much of the Doom 3 animation system as possible.
	for(int i = 0; i < md6Frames[frame.frame2].jointFrames.Num(); i++) {
		const md6JointFrame_t* jointframe = &md6Frames[frame.frame2].jointFrames[i];

		//if(jointframe->parentNum != -1) {
		//	idMat3 rotation = jointframe->rotation.ToMat3();
		//	idMat3 parentrotation = blendJoints[jointframe->parentNum].q.ToMat3();
		//
		//	blendJoints[i].q = (parentrotation.Inverse() * rotation).ToQuat();
		//}
		//else {
		//	blendJoints[i].q = jointframe->rotation;
		//}

		blendJoints[i].q = jointframe->rotation;
		blendJoints[i].q = blendJoints[i].q.Inverse();

		blendJoints[i].t = jointframe->translation;
		blendJoints[i].w = blendJoints[i].q.w;
	}

	for (int i = 0; i < numJoints; i++) {
	//	joints[i].q.Slerp(joints[i].q, blendJoints[i].q, frame.backlerp);
	//	joints[i].t.Lerp(joints[i].t, blendJoints[i].t, frame.backlerp);
		joints[i] = blendJoints[i];
	}
	

	//if (frame.cycleCount) {
	//	joints[0].t += totaldelta * (float)frame.cycleCount;
	//}
}

/*
====================
idMD5Anim::GetInterpolatedFrame
====================
*/
void idMD5Anim::GetInterpolatedFrame( frameBlend_t &frame, idJointQuat *joints, const int *index, int numIndexes ) const {
	int						i, numLerpJoints;
	const float				*frame1;
	const float				*frame2;
	const float				*jointframe1;
	const float				*jointframe2;
	const jointAnimInfo_t	*infoPtr;
	int						animBits;
	idJointQuat				*blendJoints;
	idJointQuat				*jointPtr;
	idJointQuat				*blendPtr;
	int						*lerpIndex;

// jmarshall md6 anim support
	if(animType == ANIM_TYPE_MD6) {
		GetInterpolatedMD6Frame(frame, joints, index, numIndexes);
		return;
	}
// jmarshall end

	// copy the baseframe
	SIMDProcessor->Memcpy( joints, baseFrame.Ptr(), baseFrame.Num() * sizeof( baseFrame[ 0 ] ) );

	if ( !numAnimatedComponents ) {
		// just use the base frame
		return;
	}

	blendJoints = (idJointQuat *)_alloca16( baseFrame.Num() * sizeof( blendPtr[ 0 ] ) );
	lerpIndex = (int *)_alloca16( baseFrame.Num() * sizeof( lerpIndex[ 0 ] ) );
	numLerpJoints = 0;

	frame1 = &componentFrames[ frame.frame1 * numAnimatedComponents ];
	frame2 = &componentFrames[ frame.frame2 * numAnimatedComponents ];

	for ( i = 0; i < numIndexes; i++ ) {
		int j = index[i];
		jointPtr = &joints[j];
		blendPtr = &blendJoints[j];
		infoPtr = &jointInfo[j];

		animBits = infoPtr->animBits;
		if ( animBits ) {

			lerpIndex[numLerpJoints++] = j;

			jointframe1 = frame1 + infoPtr->firstComponent;
			jointframe2 = frame2 + infoPtr->firstComponent;

			switch( animBits & (ANIM_TX|ANIM_TY|ANIM_TZ) ) {
				case 0:
					blendPtr->t = jointPtr->t;
					break;
				case ANIM_TX:
					jointPtr->t.x = jointframe1[0];
					blendPtr->t.x = jointframe2[0];
					blendPtr->t.y = jointPtr->t.y;
					blendPtr->t.z = jointPtr->t.z;
					jointframe1++;
					jointframe2++;
					break;
				case ANIM_TY:
					jointPtr->t.y = jointframe1[0];
					blendPtr->t.y = jointframe2[0];
					blendPtr->t.x = jointPtr->t.x;
					blendPtr->t.z = jointPtr->t.z;
					jointframe1++;
					jointframe2++;
					break;
				case ANIM_TZ:
					jointPtr->t.z = jointframe1[0];
					blendPtr->t.z = jointframe2[0];
					blendPtr->t.x = jointPtr->t.x;
					blendPtr->t.y = jointPtr->t.y;
					jointframe1++;
					jointframe2++;
					break;
				case ANIM_TX|ANIM_TY:
					jointPtr->t.x = jointframe1[0];
					jointPtr->t.y = jointframe1[1];
					blendPtr->t.x = jointframe2[0];
					blendPtr->t.y = jointframe2[1];
					blendPtr->t.z = jointPtr->t.z;
					jointframe1 += 2;
					jointframe2 += 2;
					break;
				case ANIM_TX|ANIM_TZ:
					jointPtr->t.x = jointframe1[0];
					jointPtr->t.z = jointframe1[1];
					blendPtr->t.x = jointframe2[0];
					blendPtr->t.z = jointframe2[1];
					blendPtr->t.y = jointPtr->t.y;
					jointframe1 += 2;
					jointframe2 += 2;
					break;
				case ANIM_TY|ANIM_TZ:
					jointPtr->t.y = jointframe1[0];
					jointPtr->t.z = jointframe1[1];
					blendPtr->t.y = jointframe2[0];
					blendPtr->t.z = jointframe2[1];
					blendPtr->t.x = jointPtr->t.x;
					jointframe1 += 2;
					jointframe2 += 2;
					break;
				case ANIM_TX|ANIM_TY|ANIM_TZ:
					jointPtr->t.x = jointframe1[0];
					jointPtr->t.y = jointframe1[1];
					jointPtr->t.z = jointframe1[2];
					blendPtr->t.x = jointframe2[0];
					blendPtr->t.y = jointframe2[1];
					blendPtr->t.z = jointframe2[2];
					jointframe1 += 3;
					jointframe2 += 3;
					break;
			}

			switch( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
				case 0:
					blendPtr->q = jointPtr->q;
					break;
				case ANIM_QX:
					jointPtr->q.x = jointframe1[0];
					blendPtr->q.x = jointframe2[0];
					blendPtr->q.y = jointPtr->q.y;
					blendPtr->q.z = jointPtr->q.z;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QY:
					jointPtr->q.y = jointframe1[0];
					blendPtr->q.y = jointframe2[0];
					blendPtr->q.x = jointPtr->q.x;
					blendPtr->q.z = jointPtr->q.z;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QZ:
					jointPtr->q.z = jointframe1[0];
					blendPtr->q.z = jointframe2[0];
					blendPtr->q.x = jointPtr->q.x;
					blendPtr->q.y = jointPtr->q.y;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QX|ANIM_QY:
					jointPtr->q.x = jointframe1[0];
					jointPtr->q.y = jointframe1[1];
					blendPtr->q.x = jointframe2[0];
					blendPtr->q.y = jointframe2[1];
					blendPtr->q.z = jointPtr->q.z;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QX|ANIM_QZ:
					jointPtr->q.x = jointframe1[0];
					jointPtr->q.z = jointframe1[1];
					blendPtr->q.x = jointframe2[0];
					blendPtr->q.z = jointframe2[1];
					blendPtr->q.y = jointPtr->q.y;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QY|ANIM_QZ:
					jointPtr->q.y = jointframe1[0];
					jointPtr->q.z = jointframe1[1];
					blendPtr->q.y = jointframe2[0];
					blendPtr->q.z = jointframe2[1];
					blendPtr->q.x = jointPtr->q.x;
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
				case ANIM_QX|ANIM_QY|ANIM_QZ:
					jointPtr->q.x = jointframe1[0];
					jointPtr->q.y = jointframe1[1];
					jointPtr->q.z = jointframe1[2];
					blendPtr->q.x = jointframe2[0];
					blendPtr->q.y = jointframe2[1];
					blendPtr->q.z = jointframe2[2];
					jointPtr->q.w = jointPtr->q.CalcW();
					blendPtr->q.w = blendPtr->q.CalcW();
					break;
			}
		}
	}

	SIMDProcessor->BlendJoints( joints, blendJoints, frame.backlerp, lerpIndex, numLerpJoints );

	if ( frame.cycleCount ) {
		joints[ 0 ].t += totaldelta * ( float )frame.cycleCount;
	}
}

/*
====================
idMD5Anim::GetSingleFrame
====================
*/
void idMD5Anim::GetSingleFrame( int framenum, idJointQuat *joints, const int *index, int numIndexes ) const {
	int						i;
	const float				*frame;
	const float				*jointframe;
	int						animBits;
	idJointQuat				*jointPtr;
	const jointAnimInfo_t	*infoPtr;

// jmarshall
	if(animType == ANIM_TYPE_MD6) {
		// This looks silly because we are trying to reuse as much of the Doom 3 animation system as possible.
		for (int i = 0; i < md6Frames[framenum].jointFrames.Num(); i++) {
			joints[i].q = md6Frames[framenum].jointFrames[i].rotation;
			joints[i].t = md6Frames[framenum].jointFrames[i].translation;
			joints[i].w = md6Frames[framenum].jointFrames[i].rotation.w;
		}

		return;
	}
// jmarshall end

	// copy the baseframe
	SIMDProcessor->Memcpy( joints, baseFrame.Ptr(), baseFrame.Num() * sizeof( baseFrame[ 0 ] ) );

	if ( ( framenum == 0 ) || !numAnimatedComponents ) {
		// just use the base frame
		return;
	}

	frame = &componentFrames[ framenum * numAnimatedComponents ];

	for ( i = 0; i < numIndexes; i++ ) {
		int j = index[i];
		jointPtr = &joints[j];
		infoPtr = &jointInfo[j];

		animBits = infoPtr->animBits;
		if ( animBits ) {

			jointframe = frame + infoPtr->firstComponent;

			if ( animBits & (ANIM_TX|ANIM_TY|ANIM_TZ) ) {

				if ( animBits & ANIM_TX ) {
					jointPtr->t.x = *jointframe++;
				}

				if ( animBits & ANIM_TY ) {
					jointPtr->t.y = *jointframe++;
				}

				if ( animBits & ANIM_TZ ) {
					jointPtr->t.z = *jointframe++;
				}
			}

			if ( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {

				if ( animBits & ANIM_QX ) {
					jointPtr->q.x = *jointframe++;
				}

				if ( animBits & ANIM_QY ) {
					jointPtr->q.y = *jointframe++;
				}

				if ( animBits & ANIM_QZ ) {
					jointPtr->q.z = *jointframe;
				}

				jointPtr->q.w = jointPtr->q.CalcW();
			}
		}
	}
}

/*
====================
idMD5Anim::CheckModelHierarchy
====================
*/
void idMD5Anim::CheckModelHierarchy( const idRenderModel *model ) const {
	int	i;
	int	jointNum;
	int	parent;

	if ( jointInfo.Num() != model->NumJoints() ) {
		gameLocal.Error( "Model '%s' has different # of joints than anim '%s'", model->Name(), name.c_str() );
	}

	const idMD5Joint *modelJoints = model->GetJoints();
	for( i = 0; i < jointInfo.Num(); i++ ) {
		jointNum = jointInfo[ i ].nameIndex;
		if ( modelJoints[ i ].name != animationLib.JointName( jointNum ) ) {
			gameLocal.Error( "Model '%s''s joint names don't match anim '%s''s", model->Name(), name.c_str() );
		}
		if ( modelJoints[ i ].parent ) {
			parent = modelJoints[ i ].parent - modelJoints;
		} else {
			parent = -1;
		}
		if ( parent != jointInfo[ i ].parentNum ) {
			gameLocal.Error( "Model '%s' has different joint hierarchy than anim '%s'", model->Name(), name.c_str() );
		}
	}
}

/***********************************************************************

	idAnimManager

***********************************************************************/

/*
====================
idAnimManager::idAnimManager
====================
*/
idAnimManager::idAnimManager() {
}

/*
====================
idAnimManager::~idAnimManager
====================
*/
idAnimManager::~idAnimManager() {
	Shutdown();
}

/*
====================
idAnimManager::Shutdown
====================
*/
void idAnimManager::Shutdown( void ) {
	animations.DeleteContents();
	jointnames.Clear();
	jointnamesHash.Free();
}

/*
====================
idAnimManager::GetAnim
====================
*/
idMD5Anim *idAnimManager::GetAnim( const char *name ) {
	idMD5Anim **animptrptr;
	idMD5Anim *anim;

	// see if it has been asked for before
	animptrptr = NULL;
	if ( animations.Get( name, &animptrptr ) ) {
		anim = *animptrptr;
	} else {
		idStr extension;
		idStr filename = name;

		filename.ExtractFileExtension( extension );
		if (extension == MD5_ANIM_EXT) {
			anim = new idMD5Anim();
			if (!anim->LoadAnim(filename)) {
				gameLocal.Warning("Couldn't load anim: '%s'", filename.c_str());
				delete anim;
				anim = NULL;
			}
		}
// jmarshall - md6anim support.
		else if(extension == MD6_ANIM_EXT) {
			anim = new idMD5Anim();
			if (!anim->LoadMD6Anim(filename)) {
				gameLocal.Warning("Couldn't load anim: '%s'", filename.c_str());
				delete anim;
				anim = NULL;
			}
		}
// jmarshall end
		else {
			return NULL;
		}
		animations.Set( filename, anim );
	}

	return anim;
}

/*
================
idAnimManager::ReloadAnims
================
*/
void idAnimManager::ReloadAnims( void ) {
	int			i;
	idMD5Anim	**animptr;

	for( i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr && *animptr ) {
			( *animptr )->Reload();
		}
	}
}

/*
================
idAnimManager::JointIndex
================
*/
int	idAnimManager::JointIndex( const char *name ) {
	int i, hash;

	hash = jointnamesHash.GenerateKey( name );
	for ( i = jointnamesHash.First( hash ); i != -1; i = jointnamesHash.Next( i ) ) {
		if ( jointnames[i].Cmp( name ) == 0 ) {
			return i;
		}
	}

	i = jointnames.Append( name );
	jointnamesHash.Add( hash, i );
	return i;
}

/*
================
idAnimManager::JointName
================
*/
const char *idAnimManager::JointName( int index ) const {
	return jointnames[ index ];
}

/*
================
idAnimManager::ListAnims
================
*/
void idAnimManager::ListAnims( void ) const {
	int			i;
	idMD5Anim	**animptr;
	idMD5Anim	*anim;
	size_t		size;
	size_t		s;
	size_t		namesize;
	int			num;

	num = 0;
	size = 0;
	for( i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr && *animptr ) {
			anim = *animptr;
			s = anim->Size();
			gameLocal.Printf( "%8d bytes : %2d refs : %s\n", s, anim->NumRefs(), anim->Name() );
			size += s;
			num++;
		}
	}

	namesize = jointnames.Size() + jointnamesHash.Size();
	for( i = 0; i < jointnames.Num(); i++ ) {
		namesize += jointnames[ i ].Size();
	}

	gameLocal.Printf( "\n%d memory used in %d anims\n", size, num );
	gameLocal.Printf( "%d memory used in %d joint names\n", namesize, jointnames.Num() );
}

/*
================
idAnimManager::FlushUnusedAnims
================
*/
void idAnimManager::FlushUnusedAnims( void ) {
	int						i;
	idMD5Anim				**animptr;
	idList<idMD5Anim *>		removeAnims;
	
	for( i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr && *animptr ) {
			if ( ( *animptr )->NumRefs() <= 0 ) {
				removeAnims.Append( *animptr );
			}
		}
	}

	for( i = 0; i < removeAnims.Num(); i++ ) {
		animations.Remove( removeAnims[ i ]->Name() );
		delete removeAnims[ i ];
	}
}
