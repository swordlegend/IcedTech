/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "engine_precompiled.h"
#include "tr_local.h"



idRenderProgManager renderProgManager;

/*
================================================================================================
idRenderProgManager::idRenderProgManager()
================================================================================================
*/
idRenderProgManager::idRenderProgManager() {
}

/*
================================================================================================
idRenderProgManager::~idRenderProgManager()
================================================================================================
*/
idRenderProgManager::~idRenderProgManager() {
}

/*
================================================================================================
R_ReloadShaders
================================================================================================
*/
static void R_ReloadShaders( const idCmdArgs &args ) {	
	renderProgManager.KillAllShaders();
	renderProgManager.LoadAllShaders();

	renderProgManager.Unbind();
}

/*
================================================================================================
idRenderProgManager::Init()
================================================================================================
*/
void idRenderProgManager::Init() {
	common->Printf( "----- Initializing Render Shaders -----\n" );


	for ( int i = 0; i < MAX_BUILTINS; i++ ) {
		builtinShaders[i] = -1;
	}
	struct builtinShaders_t {
		int index;
		const char * name;
		const char * out_name;
		const char * macros;
	} builtins[] = {
		{ BUILTIN_GUI, "gui.vfp", "gui.vfp", "" },
		{ BUILTIN_COLOR, "color.vfp", "color.vfp", "" },
		{ BUILTIN_SIMPLESHADE, "simpleshade.vfp", "simpleshade.vfp", "" },
		{ BUILTIN_TEXTURED, "texture.vfp", "texture.vfp", "" },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "texture_color.vfp", "texture_color.vfp", "" },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "texture_color_skinned.vfp", "texture_color_skinned.vfp", "" },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "texture_color_texgen.vfp", "texture_color_texgen.vfp", "" },

		// Point lights
		{ BUILTIN_INTERACTION, "interaction.vfp", "interaction.vfp", "" },
		{ BUILTIN_INTERACTION_SKINNED, "interaction.vfp", "interaction_skinned.vfp", "#define ID_GPU_SKIN\n" },

		// Spot lights.
		{ BUILTIN_INTERACTION_SPOT, "interaction.vfp", "interaction_spot.vfp", "#define ID_SPOTLIGHT\n" },
		{ BUILTIN_INTERACTION_SPOT_SKINNED, "interaction.vfp", "interaction_spot_skinned.vfp", "#define ID_GPU_SKIN\n#define ID_SPOTLIGHT\n" },
		
		// IES Spotlights.
		{ BUILTIN_INTERACTION_SPOT_IES, "interaction.vfp", "interaction_spot_ies.vfp", "#define ID_SPOTLIGHT\n #define ID_IES\n" },
		{ BUILTIN_INTERACTION_SPOT_IES_SKINNED, "interaction.vfp", "interaction_spot_ies_skinned.vfp", "#define ID_GPU_SKIN\n#define ID_SPOTLIGHT\n#define ID_IES\n" },
		
	
		{ BUILTIN_INTERACTION_AMBIENT, "interaction.vfp", "interaction.vfp", "#define ID_AMBIENT_LIGHT\n" },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "interaction.vfp", "interaction_skinned.vfp", "#define ID_GPU_SKIN\n#define ID_AMBIENT_LIGHT\n" },
		{ BUILTIN_ENVIRONMENT, "environment.vfp", "environment.vfp", "" },
		{ BUILTIN_ENVIRONMENT_SKINNED, "environment.vfp", "environment_skinned.vfp", "#define ID_GPU_SKIN\n" },
		{ BUILTIN_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp", "bumpyEnvironment.vfp", "" },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "bumpyEnvironment_skinned.vfp", "bumpyEnvironment_skinned.vfp", "" },

		{ BUILTIN_DEPTH, "depth.vfp", "depth.vfp", "" },
		{ BUILTIN_DEPTH_SKINNED, "depth.vfp", "depth_skinned.vfp", "#define ID_GPU_SKIN\n" },
		{ BUILTIN_SHADOW_DEBUG, "shadowDebug.vfp", "shadowDebug.vfp", "" },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "shadowDebug_skinned.vfp", "shadowDebug_skinned.vfp", "" },

		{ BUILTIN_BLENDLIGHT, "blendlight.vfp", "blendlight.vfp", "" },
		{ BUILTIN_FOG, "fog.vfp", "fog.vfp", "" },
		{ BUILTIN_FOG_SKINNED, "fog_skinned.vfp", "fog_skinned.vfp", "" },
		{ BUILTIN_SKYBOX, "skybox.vfp", "skybox.vfp", "" },
		{ BUILTIN_WOBBLESKY, "wobblesky.vfp", "wobblesky.vfp", "" },
		{ BUILTIN_POSTPROCESS, "postprocess.vfp", "postprocess.vfp", "" },
		{ BUILTIN_STEREO_DEGHOST, "stereoDeGhost.vfp", "stereoDeGhost.vfp", "" },
		{ BUILTIN_STEREO_WARP, "stereoWarp.vfp", "stereoWarp.vfp", "" },
		{ BUILTIN_ZCULL_RECONSTRUCT, "zcullReconstruct.vfp", "zcullReconstruct.vfp", "" },
		{ BUILTIN_BINK, "bink.vfp", "bink.vfp", "" },
		{ BUILTIN_BINK_GUI, "bink_gui.vfp", "bink_gui.vfp", "" },
		{ BUILTIN_STEREO_INTERLACE, "stereoInterlace.vfp", "stereoInterlace.vfp", "" },
		{ BUILTIN_MOTION_BLUR, "motionBlur.vfp", "motionBlur.vfp", "" },
		{ BUILTIN_TESTIMAGE, "testimage.vfp", "testimage.vfp", "" },
		{ BUILDIN_VIRTUALTEXTURE_FEEDBACK, "feedback.vfp", "feedback.vfp", "" },
		{ BUILDIN_VIRTUALTEXTURE_FEEDBACK_SKINNING, "feedback.vfp", "feedback_skinning.vfp", "#define ID_GPU_SKIN\n" },
		{ BUILTIN_SHADOW, "shadow.vfp", "shadow.vfp", ""},
		{ BUILTIN_SHADOW_SKINNED, "shadow.vfp", "shadow_skinned.vfp", "#define ID_GPU_SKIN\n"},
		{ BUILTIN_SKYCOLOR, "sky_color.vfp", "sky_color.vfp", ""},
		{ BUILTIN_PARTICLE, "particle.vfp", "particle.vfp", ""},
		{ BUILTIN_PARTICLE_BLOOM, "particle.vfp", "particle_bloom.vfp", "#define ID_PARTICLE_BLOOM\n"}
	};
	int numBuiltins = sizeof( builtins ) / sizeof( builtins[0] );
	vertexShaders.SetNum( numBuiltins );
	fragmentShaders.SetNum( numBuiltins );
	glslPrograms.SetNum( numBuiltins );

	for ( int i = 0; i < numBuiltins; i++ ) {
		vertexShaders[i].name = builtins[i].name;
		vertexShaders[i].out_name = builtins[i].out_name;
		vertexShaders[i].macros = builtins[i].macros;
		fragmentShaders[i].name = builtins[i].name;
		fragmentShaders[i].out_name = builtins[i].out_name;
		fragmentShaders[i].macros = builtins[i].macros;
		builtinShaders[builtins[i].index] = i;
		LoadVertexShader( i );
		LoadFragmentShader( i );
		LoadGLSLProgram( i, i, i );
	}

	glslUniforms.SetNum( RENDERPARM_USER + MAX_GLSL_USER_PARMS );

	vertexShaders[builtinShaders[BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_INTERACTION_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_INTERACTION_AMBIENT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_ENVIRONMENT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_BUMPY_ENVIRONMENT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_DEPTH_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_SHADOW_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_SHADOW_DEBUG_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_FOG_SKINNED]].usesJoints = true;

	glUseProgram(0);

	cmdSystem->AddCommand( "reloadShaders", R_ReloadShaders, CMD_FL_RENDERER, "reloads shaders" );
}

/*
================================================================================================
idRenderProgManager::LoadAllShaders()
================================================================================================
*/
void idRenderProgManager::LoadAllShaders() {
	for ( int i = 0; i < vertexShaders.Num(); i++ ) {
		LoadVertexShader( i );
	}
	for ( int i = 0; i < fragmentShaders.Num(); i++ ) {
		LoadFragmentShader( i );
	}

	for ( int i = 0; i < glslPrograms.Num(); ++i ) {
		LoadGLSLProgram( i, glslPrograms[i].vertexShaderIndex, glslPrograms[i].fragmentShaderIndex );
	}
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders() {
	Unbind();
	for ( int i = 0; i < vertexShaders.Num(); i++ ) {
		if ( vertexShaders[i].progId != INVALID_PROGID ) {
			glDeleteShader( vertexShaders[i].progId );
			vertexShaders[i].progId = INVALID_PROGID;
		}
	}
	for ( int i = 0; i < fragmentShaders.Num(); i++ ) {
		if ( fragmentShaders[i].progId != INVALID_PROGID ) {
			glDeleteShader( fragmentShaders[i].progId );
			fragmentShaders[i].progId = INVALID_PROGID;
		}
	}
	for ( int i = 0; i < glslPrograms.Num(); ++i ) {
		if ( glslPrograms[i].progId != INVALID_PROGID ) {
			glDeleteProgram( glslPrograms[i].progId );
			glslPrograms[i].progId = INVALID_PROGID;
		}
	}
}

/*
================================================================================================
idRenderProgManager::Shutdown()
================================================================================================
*/
void idRenderProgManager::Shutdown() {
	KillAllShaders();
}

/*
================================================================================================
idRenderProgManager::FindVertexShader
================================================================================================
*/
int idRenderProgManager::FindVertexShader( const char * name, const char* programInstanceOutName, const char* programInstanceMacros) {
	for ( int i = 0; i < vertexShaders.Num(); i++ ) {
		idStr actualShaderName = name;
		if(vertexShaders[i].out_name != NULL) {
			actualShaderName = vertexShaders[i].out_name;
		}

		if (programInstanceOutName == NULL) {
			if (actualShaderName.Icmp(name) == 0) {
				LoadVertexShader(i);
				return i;
			}
		}
		else {
			if (actualShaderName.Icmp(programInstanceOutName) == 0) {
				LoadVertexShader(i);
				return i;
			}
		}
	}
	vertexShader_t shader;
	shader.name = name;
	shader.out_name = programInstanceOutName;
	shader.macros = programInstanceMacros;

	int index = vertexShaders.Append( shader );
	LoadVertexShader( index );
	currentVertexShader = index;

	// FIXME: we should really scan the program source code for using rpEnableSkinning but at this
	// point we directly load a binary and the program source code is not available on the consoles
	if (	idStr::Icmp( name, "heatHaze.vfp" ) == 0 ||
			idStr::Icmp( name, "heatHazeWithMask.vfp" ) == 0 ||
			idStr::Icmp( name, "heatHazeWithMaskAndVertex.vfp" ) == 0 ) {
		vertexShaders[index].usesJoints = true;
		vertexShaders[index].optionalSkinning = true;
	}

	return index;
}

/*
================================================================================================
idRenderProgManager::FindFragmentShader
================================================================================================
*/
int idRenderProgManager::FindFragmentShader( const char * name, const char* programInstanceOutName, const char* programInstanceMacros) {
	for ( int i = 0; i < fragmentShaders.Num(); i++ ) {
		idStr actualShaderName = name;
		if (fragmentShaders[i].out_name != NULL) {
			actualShaderName = fragmentShaders[i].out_name;
		}

		if (programInstanceOutName == NULL) {
			if (actualShaderName.Icmp(name) == 0) {
				LoadFragmentShader(i);
				return i;
			}
		}
		else {
			if (actualShaderName.Icmp(programInstanceOutName) == 0) {
				LoadFragmentShader(i);
				return i;
			}
		}
	}
	fragmentShader_t shader;
	shader.name = name;
	shader.out_name = programInstanceOutName;
	shader.macros = programInstanceMacros;
	int index = fragmentShaders.Append( shader );
	LoadFragmentShader( index );
	currentFragmentShader = index;
	return index;
}




/*
================================================================================================
idRenderProgManager::LoadVertexShader
================================================================================================
*/
void idRenderProgManager::LoadVertexShader( int index ) {
	if ( vertexShaders[index].progId != INVALID_PROGID ) {
		return; // Already loaded
	}

	if (vertexShaders[index].out_name == NULL) {
		vertexShaders[index].out_name = vertexShaders[index].name;
	}

	if (vertexShaders[index].macros == NULL) {
		vertexShaders[index].macros = "";
	}


	vertexShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_VERTEX_SHADER, vertexShaders[index].name, vertexShaders[index].out_name, vertexShaders[index].macros, vertexShaders[index].uniforms );
}

/*
================================================================================================
idRenderProgManager::LoadFragmentShader
================================================================================================
*/
void idRenderProgManager::LoadFragmentShader( int index ) {
	if ( fragmentShaders[index].progId != INVALID_PROGID ) {
		return; // Already loaded
	}

	if (fragmentShaders[index].out_name == NULL) {
		fragmentShaders[index].out_name = fragmentShaders[index].name;
	}

	if (fragmentShaders[index].macros == NULL) {
		fragmentShaders[index].macros = "";
	}

	fragmentShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_FRAGMENT_SHADER, fragmentShaders[index].name, fragmentShaders[index].out_name, fragmentShaders[index].macros,  fragmentShaders[index].uniforms );
}

/*
================================================================================================
idRenderProgManager::LoadShader
================================================================================================
*/
GLuint idRenderProgManager::LoadShader( GLenum target, const char * name, const char * startToken ) {

	idStr fullPath = "renderprogs\\gl\\";
	fullPath += name;

	common->Printf( "%s", fullPath.c_str() );

	char * fileBuffer = NULL;
	fileSystem->ReadFile( fullPath.c_str(), (void **)&fileBuffer, NULL );
	if ( fileBuffer == NULL ) {
		common->Printf( ": File not found\n" );
		return INVALID_PROGID;
	}
	if ( !renderSystem->IsOpenGLRunning() ) {
		common->Printf( ": Renderer not initialized\n" );
		fileSystem->FreeFile( fileBuffer );
		return INVALID_PROGID;
	}

	// vertex and fragment shaders are both be present in a single file, so
	// scan for the proper header to be the start point, and stamp a 0 in after the end
	char * start = strstr( (char *)fileBuffer, startToken );
	if ( start == NULL ) {
		common->Printf( ": %s not found\n", startToken );
		fileSystem->FreeFile( fileBuffer );
		return INVALID_PROGID;
	}
	char * end = strstr( start, "END" );
	if ( end == NULL ) {
		common->Printf( ": END not found for %s\n", startToken );
		fileSystem->FreeFile( fileBuffer );
		return INVALID_PROGID;
	}
	end[3] = 0;

	idStr program = start;
	program.Replace( "vertex.normal", "vertex.attrib[11]" );
	program.Replace( "vertex.texcoord[0]", "vertex.attrib[8]" );
	program.Replace( "vertex.texcoord", "vertex.attrib[8]" );

	GLuint progId;
	glGenProgramsARB( 1, &progId );

	glBindProgramARB( target, progId );
	glGetError();

	glProgramStringARB( target, GL_PROGRAM_FORMAT_ASCII_ARB, program.Length(), program.c_str() );
	GLenum err = glGetError();

	GLint ofs = -1;
	glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &ofs );
	if ( ( err == GL_INVALID_OPERATION ) || ( ofs != -1 ) ) {
		if ( err == GL_INVALID_OPERATION ) {
			const GLubyte * str = glGetString( GL_PROGRAM_ERROR_STRING_ARB );
			common->Printf( "\nGL_PROGRAM_ERROR_STRING_ARB: %s\n", str );
		} else {
			common->Printf( "\nUNKNOWN ERROR\n" );
		}
		if ( ofs < 0 ) {
			common->Printf( "GL_PROGRAM_ERROR_POSITION_ARB < 0\n" );
		} else if ( ofs >= program.Length() ) {
			common->Printf( "error at end of shader\n" );
		} else {
			common->Printf( "error at %i:\n%s", ofs, program.c_str() + ofs );
		}
		glDeleteProgramsARB( 1, &progId );
		fileSystem->FreeFile( fileBuffer );
		return INVALID_PROGID;
	}
	common->Printf( "\n" );
	fileSystem->FreeFile( fileBuffer );
	return progId;
}

/*
================================================================================================
idRenderProgManager::BindShader
================================================================================================
*/
void idRenderProgManager::BindShader( int vIndex, int fIndex ) {
	if ( currentVertexShader == vIndex && currentFragmentShader == fIndex ) {
		return;
	}
	currentVertexShader = vIndex;
	currentFragmentShader = fIndex;
	// vIndex denotes the GLSL program
	if ( vIndex >= 0 && vIndex < glslPrograms.Num() ) {
		currentRenderProgram = vIndex;
//		RENDERLOG_PRINTF( "Binding GLSL Program %s\n", glslPrograms[vIndex].name.c_str() );
		glUseProgram( glslPrograms[vIndex].progId );
	}
}

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind() {
	currentVertexShader = -1;
	currentFragmentShader = -1;
	currentRenderProgram = -1;

	glUseProgram( 0 );
}

/*
================================================================================================
idRenderProgManager::SetRenderParms
================================================================================================
*/
void idRenderProgManager::SetRenderParms( renderParm_t rp, const float * value, int num ) {
	for ( int i = 0; i < num; i++ ) {
		SetRenderParm( (renderParm_t)(rp + i), value + ( i * 4 ) );
	}
}

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm( renderParm_t rp, const float * value ) {
	SetUniformValue( rp, value );
}

