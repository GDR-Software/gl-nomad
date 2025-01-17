/*
===========================================================================
Copyright (C) 2023-2024 GDR Games

This file is part of The Nomad source code.

The Nomad source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

The Nomad source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#include "rgl_local.h"
#include "rgl_fbo.h"
#include <pthread.h>

qboolean screenshotFrame;

void GL_SetObjectDebugName( GLenum target, GLuint id, const char *name, const char *add )
{
	if ( r_glDebug->i ) {
		static char newName[1024];
		Com_snprintf( newName, sizeof(newName), "%s%s", name, add );
		nglObjectLabel( target, id, strlen(newName), newName );
	}
}

static GLuint textureStack[MAX_TEXTURE_UNITS];
static GLint textureStackP;

void GL_PushTexture( texture_t *image )
{
	uint32_t i;
	GLuint id = image ? image->id : 0;

	// do we need to pop one?
	if ( textureStackP == MAX_TEXTURE_UNITS - 1 ) {
		GL_PopTexture();
	}

	// check if it's already bound
	for ( i = 0; i < textureStackP; i++ ) {
		if ( textureStack[i] == id ) {
			textureStackP = i;
			break;
		}
	}

	if ( i == textureStackP ) {
		if ( textureStackP >= MAX_TEXTURE_UNITS ) {
			ri.Error( ERR_DROP, "GL_PushTexture: texture stack overflow" );
		}

		textureStack[ textureStackP++ ] = id;
		
		if ( !r_loadTexturesOnDemand->i ) {
			nglActiveTexture( GL_TEXTURE0 + textureStackP );
			nglBindTexture( GL_TEXTURE_2D, id );
		}
	}
}

void GL_PopTexture( void )
{
	textureStackP--;

	if ( textureStackP < 0 ) {
		ri.Error( ERR_DROP, "GL_PopTexture: texture stack underflow" );
	}

	if ( textureStackP ) {
		nglActiveTexture( GL_TEXTURE + textureStackP );
		nglBindTexture( GL_TEXTURE_2D, textureStack[ textureStackP - 1 ] );
	}
}

void GL_BindTexture( int tmu, texture_t *image )
{
	GLuint texunit = GL_TEXTURE0 + tmu;
	GLuint id = image ? image->id : 0;

	if ( image ) {
		R_TouchTexture( image );
	}
	if ( r_loadTexturesOnDemand->i ) {
		return;
	}

	if ( glState.currenttextures[tmu] == id ) {
		return;
	}

	if ( glState.currenttmu != texunit ) {
		nglActiveTexture( texunit );
		glState.currenttmu = texunit;
	}

	nglBindTexture( GL_TEXTURE_2D, id );
}

void GL_BindNullTextures( void )
{
	uint32_t i;
	for ( i = 0; i < MAX_TEXTURE_UNITS; i++ ) {
		nglActiveTexture( GL_TEXTURE0 + i );
		nglBindTexture( GL_TEXTURE_2D, 0 );
	}

	memset(textureStack, 0, sizeof(GLuint) * textureStackP);
	textureStackP = 0;

#if 0
	for (uint32_t i = 0; i < NUM_TEXTURE_BINDINGS; i++) {
		if (glState.currenttextures[i] != 0) {
			nglActiveTexture(GL_TEXTURE0 + i);
			nglBindTexture(GL_TEXTURE_2D, 0);
		}
	}
#endif
	nglActiveTexture(GL_TEXTURE0);
	glState.currenttmu = GL_TEXTURE0;
	glState.currentTexture = NULL;
}

int GL_UseProgram( GLuint program ) 
{
	if ( glState.shaderId == program ) {
		return 0;
	}
	
	nglUseProgram( program );
	glState.shaderId = program;
	return 1;
}

void GL_BindNullProgram( void )
{
	nglUseProgram( (unsigned)0 );
	glState.shaderId = 0;
}

void GL_BindFramebuffer( GLenum target, GLuint fbo )
{
	switch ( target ) {
	case GL_FRAMEBUFFER:
		if (glState.defFboId == fbo)
			return;
		
		glState.defFboId = fbo;
		break;
	case GL_READ_FRAMEBUFFER:
		if (glState.readFboId == fbo)
			return;
		
		glState.readFboId = fbo;
		break;
	case GL_DRAW_FRAMEBUFFER:
		if (glState.writeFboId == fbo)
			return;
		
		glState.writeFboId = fbo;
		break;
	default: // should never happen, if it does, then skill issue from the dev
		ri.Error(ERR_FATAL, "GL_BindFramebuffer: Invalid fbo target: %i", target);
	};

	nglBindFramebuffer( target, fbo );
}

void GL_BindNullFramebuffers( void )
{
	nglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	nglBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
	nglBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
	glState.currentFbo = NULL;
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( uint32_t stateBits )
{
	uint32_t diff = stateBits ^ glState.glStateBits;

	if ( !diff ) {
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_BITS ) {
		if ( stateBits & GLS_DEPTHFUNC_EQUAL ) {
			nglDepthFunc( GL_EQUAL );
		}
		else if ( stateBits & GLS_DEPTHFUNC_GREATER ) {
			nglDepthFunc( GL_GREATER );
		}
		else {
			nglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
		uint32_t oldState = glState.glStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
		uint32_t newState = stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
		uint32_t storedState = glState.storedGlState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );

		if ( oldState == 0 ) {
			nglEnable( GL_BLEND );
		}
		else if ( newState == 0 ) {
			nglDisable( GL_BLEND );
		}

		if ( newState != 0 && storedState != newState ) {
			GLenum srcFactor = GL_ONE, dstFactor = GL_ONE;

			glState.storedGlState &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
			glState.storedGlState |= newState;

			switch ( stateBits & GLS_SRCBLEND_BITS ) {
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				ri.Error( ERR_DROP, "GL_State: invalid src blend state bits" );
				break;
			};

			switch ( stateBits & GLS_DSTBLEND_BITS ) {
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
				break;
			};

			nglBlendFunc( srcFactor, dstFactor );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			nglDepthMask( GL_TRUE );
		}
		else
		{
			nglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			nglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			nglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			nglDisable( GL_DEPTH_TEST );
		}
		else
		{
			nglEnable( GL_DEPTH_TEST );
		}
	}

	glState.glStateBits = stateBits;
}


const char *GL_ErrorString( GLenum error )
{
	switch ( error ) {
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_INVALID_INDEX:
		return "GL_INVALID_INDEX";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	default:
		break;
	};
	return "Unknown Error Code";
}

void GL_CheckErrors( void )
{
	GLenum error = nglGetError();

	if (error == GL_NO_ERROR)
		return;
	
	if (!r_ignoreGLErrors->i) {
		ri.Printf(PRINT_INFO, COLOR_RED "GL_CheckErrors: 0x%04x -- %s\n", error, GL_ErrorString(error));
		ri.Error(ERR_FATAL, "GL_CheckErrors: OpenGL error occured (0x%x): %s", error, GL_ErrorString(error));
	}
}

void GDR_ATTRIBUTE((format(printf, 1, 2))) GL_LogComment(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	int length;

	va_start(argptr, fmt);
	length = N_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	nglDebugMessageInsertARB(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DEBUG_TYPE_OTHER_ARB, 0, GL_DEBUG_SEVERITY_NOTIFICATION, length, msg);
}

void GDR_ATTRIBUTE((format(printf, 1, 2))) GL_LogError(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	int length;

	va_start(argptr, fmt);
	length = N_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	nglDebugMessageInsertARB(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DEBUG_TYPE_ERROR_ARB, 0, GL_DEBUG_SEVERITY_HIGH_ARB, length, msg);
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	uint32_t    i;
	texture_t   *image;
	float	    x, y, w, h;
	uint64_t    start, end;

	nglClear( GL_COLOR_BUFFER_BIT );

	nglFinish();

	start = ri.Milliseconds();

	for ( i=0 ; i<rg.numTextures ; i++ ) {
		image = rg.textures[i];

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->i == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		{
			vec4_t quadVerts[4];

			GL_BindTexture(TB_COLORMAP, image);
			RB_RenderPass();
		}
	}

	nglFinish();

	end = ri.Milliseconds();
	ri.Printf( PRINT_INFO, "%lu msec to draw all images\n", end - start );
}

static const void *RB_SwapBuffers( const void *data )
{
	const swapBuffersCmd_t *cmd;
	uint64_t start, end;
	
	// texture swapping test
	if ( r_showImages->i ) {
		RB_ShowImages();
	}
	
	// only draw imgui data after everything else has finished
//	if ( !backend.framePostProcessed ) {
		ri.ImGui_Draw();
		GL_CheckErrors();
//	}
	
	cmd = (const swapBuffersCmd_t *)data;

	if ( r_glDiagnostics->i ) {
		if ( rg.beganQuery ) {
			nglEndQuery( GL_TIME_ELAPSED );
			nglEndQuery( GL_SAMPLES_PASSED );
			nglEndQuery( GL_PRIMITIVES_GENERATED );

			nglGetQueryObjectiv( rg.queries[TIME_QUERY], GL_QUERY_RESULT, &rg.queryCounts[TIME_QUERY] );
			nglGetQueryObjectiv( rg.queries[SAMPLES_QUERY], GL_QUERY_RESULT, &rg.queryCounts[SAMPLES_QUERY] );
			nglGetQueryObjectiv( rg.queries[PRIMTIVES_QUERY], GL_QUERY_RESULT, &rg.queryCounts[PRIMTIVES_QUERY] );

			rg.beganQuery = qfalse;
		}
	}

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->i ) {
		uint32_t i;
		uint64_t sum = 0;
		byte *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		nglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[i];
		}

		backend.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory( stencilReadback );
	}
	
	if ( glContext.ARB_framebuffer_object && r_arb_framebuffer_object->i ) {
		if ( !backend.framePostProcessed ) {
			start = ri.Milliseconds();
			if ( rg.msaaResolveFbo.frameBuffer && r_multisampleType->i == AntiAlias_MSAA ) {
				// Resolving an RGB16F MSAA FBO to the screen messes with the brightness, so resolve to an RGB16F FBO first
				FBO_FastBlit( &rg.renderFbo, NULL, &rg.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
				RB_BloomPass( &rg.msaaResolveFbo, &rg.msaaResolveFbo );
				
				// FIXME: very hacky way of getting around the whole upscaling with msaa thing
				GL_BindFramebuffer( GL_FRAMEBUFFER, rg.msaaResolveFbo.frameBuffer );

				GLSL_UseProgram( &rg.bloomResolveShader );
				GLSL_SetUniformInt( &rg.bloomResolveShader, UNIFORM_USE_HDR, r_hdr->i );
				GL_BindTexture( UNIFORM_DIFFUSE_MAP, rg.msaaResolveFbo.colorImage[ 0 ] );
				GLSL_SetUniformTexture( &rg.bloomResolveShader, UNIFORM_DIFFUSE_MAP, rg.msaaResolveFbo.colorImage[ 0 ] );
				GLSL_SetUniformFloat( &rg.bloomResolveShader, UNIFORM_EXPOSURE, r_autoExposure->f );
				GLSL_SetUniformFloat( &rg.bloomResolveShader, UNIFORM_GAMMA, r_gammaAmount->f );

				RB_RenderPass();

				if ( r_fixedRendering->i ) {
					GL_BindFramebuffer( GL_FRAMEBUFFER, 0 );
					nglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
					nglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );

					ivec4_t dstBox;

					dstBox[0] = 0;
					dstBox[1] = 0;
					dstBox[2] = glConfig.vidWidth;
					dstBox[3] = glConfig.vidHeight;

					FBO_FastBlit( &rg.msaaResolveFbo, NULL, NULL, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
				}
				else {
					FBO_FastBlit( &rg.msaaResolveFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST );
				}
			}
			else if ( rg.ssaaResolveFbo.frameBuffer && r_multisampleType->i == AntiAlias_SSAA ) {
				ivec4_t dstBox;

				dstBox[0] = 0;
				dstBox[1] = 0;
				dstBox[2] = rg.renderFbo.width;
				dstBox[3] = rg.renderFbo.height;

				FBO_FastBlit( &rg.renderFbo, NULL, &rg.ssaaResolveFbo, dstBox, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
//				FBO_FastBlit( &rg.ssaaResolveFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST );
				RB_FinishPostProcess( &rg.ssaaResolveFbo );
			}
			else {
				RB_BloomPass( &rg.renderFbo, &rg.renderFbo );
				RB_FinishPostProcess( &rg.renderFbo );
//				FBO_FastBlit( &rg.renderFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
			}
			end = ri.Milliseconds();
			backend.pc.postprocessMsec = end - start;
		}
	}

	if ( !glState.finishCalled ) {
		nglFinish();
	}

	if ( screenshotFrame ) {
		RB_TakeScreenshotCmd();
		screenshotFrame = qfalse;
	}

	ri.GLimp_EndFrame();

	backend.framePostProcessed = qfalse;

	return (const void *)( cmd + 1 );
}

/*
=============
RB_PostProcess

=============
*/
static const void *RB_PostProcess( const void *data )
{
	const postProcessCmd_t *cmd = data;
	fbo_t *srcFbo;
	ivec4_t srcBox, dstBox;
	qboolean autoExposure;
	uint64_t start, end;

	start = ri.Milliseconds();

	backend.refdef.blurFactor = 0.0f;
	backend.drawBatch.shader = rg.defaultShader;

	// finish any drawing if needed
	if ( backend.drawBatch.idxOffset ) {
		RB_FlushBatchBuffer();
	}

	// only draw imgui data after everything else has finished
//	if ( !backend.framePostProcessed ) {
		ri.ImGui_Draw();
		GL_CheckErrors();
//	}

	if ( !glContext.ARB_framebuffer_object || !r_postProcess->i ) {
		// do nothing
		return (const void *)( cmd + 1 );
	}

	if ( cmd ) {
		backend.refdef = cmd->refdef;
		glState.viewData = cmd->viewData;
	}

	srcFbo = &rg.renderFbo;
	if ( rg.msaaResolveFbo.frameBuffer && r_multisampleType->i == AntiAlias_MSAA ) {
		// Resolve the MSAA before anything else
		// Can't resolve just part of the MSAA FBO, so multiple views will suffer a performance hit here
		// Resolving an RGB16F MSAA FBO to the screen messes with the brightness, so resolve to an RGB16F FBO first
		FBO_FastBlit( &rg.renderFbo, NULL, &rg.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
		srcFbo = &rg.msaaResolveFbo;
	} else if ( rg.ssaaResolveFbo.frameBuffer && r_multisampleType->i == AntiAlias_SSAA ) {
		ivec4_t dstBox;

		dstBox[0] = 0;
		dstBox[1] = 0;
		dstBox[2] = rg.renderFbo.width;
		dstBox[3] = rg.renderFbo.height;

		FBO_FastBlit( &rg.renderFbo, NULL, &rg.ssaaResolveFbo, dstBox, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
		srcFbo = &rg.ssaaResolveFbo;
	}

	dstBox[0] = glState.viewData.viewportX;
	dstBox[1] = glState.viewData.viewportY;
	dstBox[2] = glState.viewData.viewportWidth;
	dstBox[3] = glState.viewData.viewportHeight;

	srcBox[0] = glState.viewData.viewportX;
	srcBox[1] = glState.viewData.viewportY;
	srcBox[2] = glState.viewData.viewportWidth;
	srcBox[3] = glState.viewData.viewportHeight;

	if ( r_bloom->i ) {
		if ( r_multisampleType->i == AntiAlias_MSAA ) {
			// FIXME: very hacky way of getting around the whole upscaling with msaa thing
			GL_BindFramebuffer( GL_FRAMEBUFFER, rg.msaaResolveFbo.frameBuffer );

			GLSL_UseProgram( &rg.bloomResolveShader );
			GLSL_SetUniformInt( &rg.bloomResolveShader, UNIFORM_USE_HDR, r_hdr->i );
			GL_BindTexture( UNIFORM_DIFFUSE_MAP, rg.msaaResolveFbo.colorImage[ 0 ] );
			GLSL_SetUniformTexture( &rg.bloomResolveShader, UNIFORM_DIFFUSE_MAP, rg.msaaResolveFbo.colorImage[ 0 ] );
			GLSL_SetUniformFloat( &rg.bloomResolveShader, UNIFORM_EXPOSURE, r_autoExposure->f );
			GLSL_SetUniformFloat( &rg.bloomResolveShader, UNIFORM_GAMMA, r_gammaAmount->f );

			RB_RenderPass();
		} else {
			RB_BloomPass( srcFbo, srcFbo );
		}
	}

	if ( r_multisampleType->i == AntiAlias_MSAA ) {
		GL_BindFramebuffer( GL_FRAMEBUFFER, 0 );
		nglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
		nglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );

		ivec4_t dstBox;

		dstBox[0] = 0;
		dstBox[1] = 0;
		dstBox[2] = glConfig.vidWidth;
		dstBox[3] = glConfig.vidHeight;

		FBO_FastBlit( &rg.msaaResolveFbo, NULL, NULL, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );
	} else {
		RB_FinishPostProcess( srcFbo );
	}

#if 0
	if (0)
	{
		vec4_t quadVerts[4];
		vec2_t texCoords[4];
		ivec4_t iQtrBox;
		vec4_t box;
		vec4_t viewInfo;
		static float scale = 5.0f;

		scale -= 0.005f;
		if (scale < 0.01f)
			scale = 5.0f;

		FBO_FastBlit(NULL, NULL, rg.quarterFbo[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		iQtrBox[0] = glState.viewData.viewportX      * rg.quarterImage[0]->width / (float)glConfig.vidWidth;
		iQtrBox[1] = glState.viewData.viewportY      * rg.quarterImage[0]->height / (float)glConfig.vidHeight;
		iQtrBox[2] = glState.viewData.viewportWidth  * rg.quarterImage[0]->width / (float)glConfig.vidWidth;
		iQtrBox[3] = glState.viewData.viewportHeight * rg.quarterImage[0]->height / (float)glConfig.vidHeight;

		qglViewport(iQtrBox[0], iQtrBox[1], iQtrBox[2], iQtrBox[3]);
		qglScissor(iQtrBox[0], iQtrBox[1], iQtrBox[2], iQtrBox[3]);

		VectorSet4(box, 0.0f, 0.0f, 1.0f, 1.0f);

		texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
		texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
		texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
		texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

		VectorSet4(box, -1.0f, -1.0f, 1.0f, 1.0f);

		VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
		VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
		VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
		VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

		GL_State(GLS_DEPTHTEST_DISABLE);


		VectorSet4(viewInfo, glState.viewData.zFar / r_znear->f, glState.viewData.zFar, 0.0, 0.0);

		viewInfo[2] = scale / (float)(rg.quarterImage[0]->width);
		viewInfo[3] = scale / (float)(rg.quarterImage[0]->height);

		FBO_Bind(rg.quarterFbo[1]);
		GLSL_BindProgram(&rg.depthBlurShader[2]);
		GL_BindToTMU(rg.quarterImage[0], TB_COLORMAP);
		GLSL_SetUniformVec4(&rg.depthBlurShader[2], UNIFORM_VIEWINFO, viewInfo);
		RB_InstantQuad2(quadVerts, texCoords);

		FBO_Bind(rg.quarterFbo[0]);
		GLSL_BindProgram(&rg.depthBlurShader[3]);
		GL_BindToTMU(rg.quarterImage[1], TB_COLORMAP);
		GLSL_SetUniformVec4(&rg.depthBlurShader[3], UNIFORM_VIEWINFO, viewInfo);
		RB_InstantQuad2(quadVerts, texCoords);

		SetViewportAndScissor();

		FBO_FastBlit(rg.quarterFbo[1], NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		FBO_Bind(NULL);
	}

	if (0 && r_sunlightMode->i)
	{
		ivec4_t dstBox;
		VectorSet4(dstBox, 0, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(rg.sunShadowDepthImage[0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 128, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(rg.sunShadowDepthImage[1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 256, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(rg.sunShadowDepthImage[2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 384, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(rg.sunShadowDepthImage[3], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0 && r_shadows->i == 4)
	{
		ivec4_t dstBox;
		VectorSet4(dstBox, 512 + 0, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(rg.pshadowMaps[0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512 + 128, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(rg.pshadowMaps[1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512 + 256, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(rg.pshadowMaps[2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512 + 384, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(rg.pshadowMaps[3], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if ( 0 ) {
		ivec4_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(rg.renderDepthImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(rg.screenShadowImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if ( 0 ) {
		ivec4_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(rg.sunRaysImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (r_cubeMapping->i && rg.numCubemaps)
	{
		ivec4_t dstBox;
		int cubemapIndex = R_CubemapForPoint( glState.viewData.or.origin );

		if (cubemapIndex)
		{
			VectorSet4(dstBox, 0, glConfig.vidHeight - 256, 256, 256);
			//FBO_BlitFromTexture(rg.renderCubeImage, NULL, NULL, NULL, dstBox, &rg.testcubeShader, NULL, 0);
			FBO_BlitFromTexture(rg.cubemaps[cubemapIndex - 1].image, NULL, NULL, NULL, dstBox, &rg.testcubeShader, NULL, 0);
		}
	}
#endif

	end = ri.Milliseconds();
	backend.pc.postprocessMsec = end - start;

	backend.framePostProcessed = qtrue;

	return (const void *)(cmd + 1);
}

/*
=============
RB_DrawBuffer

=============
*/
static const void	*RB_DrawBuffer( const void *data ) {
	const drawBufferCmd_t *cmd;

	cmd = (const drawBufferCmd_t *)data;

	// finish any drawing if needed
	RB_FlushBatchBuffer();

	if ( glContext.ARB_framebuffer_object ) {
		FBO_Bind( NULL );
	}

	nglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->i ) {
		nglClearColor( 1, 0, 0.5, 1 );
		nglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)( cmd + 1 );
}

static const void *RB_SetColor(const void *data) {
	const setColorCmd_t *cmd;

	cmd = (const setColorCmd_t *)data;

	backend.color2D[0] = cmd->color[0]/* * 255*/;
	backend.color2D[1] = cmd->color[1]/* * 255*/;
	backend.color2D[2] = cmd->color[2]/* * 255*/;
	backend.color2D[3] = cmd->color[3]/* * 255*/;

	return (const void *)( cmd + 1 );
}

static const void *RB_ColorMask(const void *data) {
	const colorMaskCmd_t *cmd;

	cmd = (const colorMaskCmd_t *)data;

	// finish any drawing if needed
	RB_FlushBatchBuffer();
	
	if ( glContext.ARB_framebuffer_object ) {
		// reverse color mask, so 0 0 0 0 is the default
		backend.colorMask[0] = !cmd->rgba[0];
		backend.colorMask[1] = !cmd->rgba[1];
		backend.colorMask[2] = !cmd->rgba[2];
		backend.colorMask[3] = !cmd->rgba[3];
	}

	nglColorMask( cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3] );

	return (const void *)( cmd + 1 );
}

/*
=============
RB_ClearDepth

=============
*/
static const void *RB_ClearDepth( const void *data )
{
	const clearDepthCommand_t *cmd = data;
	
	// finish any 2D drawing if needed
	if ( backend.drawBatch.vtxOffset ) {
		RB_FlushBatchBuffer();
	}

	// texture swapping test
	if ( r_showImages->i ) {
		RB_ShowImages();
	}

	if ( glContext.ARB_framebuffer_object ) {
		if ( !rg.renderFbo.frameBuffer || backend.framePostProcessed ) {
			FBO_Bind( NULL );
		} else {
			FBO_Bind( &rg.renderFbo );
		}
	}

	nglClear( GL_DEPTH_BUFFER_BIT );

	// if we're doing MSAA, clear the depth texture for the resolve buffer
	if ( rg.msaaResolveFbo.frameBuffer ) {
		FBO_Bind( &rg.msaaResolveFbo );
		nglClear( GL_DEPTH_BUFFER_BIT );
	}

	
	return (const void *)( cmd + 1 );
}

static const void *RB_DrawImage( const void *data ) {
	const drawImageCmd_t *cmd;
	shader_t *shader;
	srfVert_t *verts;
	glIndex_t *indices;
	uint32_t numVerts, numIndices;
	int i;

	cmd = (const drawImageCmd_t *)data;

	shader = cmd->shader;
	if ( backend.drawBatch.shader != shader ) {
		if ( backend.drawBatch.idxOffset ) {
			RB_FlushBatchBuffer();
		}
		RB_SetBatchBuffer( backend.drawBuffer[ backend.cpuBuffer ], backendData[ rg.smpFrame ]->verts, sizeof( srfVert_t ),
			backendData[ rg.smpFrame ]->indices, sizeof( glIndex_t ) );
	}
	backend.drawBatch.shader = shader;

	verts = backendData[ rg.smpFrame ]->verts;
	indices = backendData[ rg.smpFrame ]->indices;
	numVerts = backend.drawBatch.vtxOffset;
	numIndices = backend.drawBatch.idxOffset;

	backend.drawBatch.vtxOffset += 4;
	backend.drawBatch.idxOffset += 6;

	{
		uint16_t color[4];

//		VectorScale4( backend.color2D, 257, color );

		VectorCopy4( backendData[ rg.smpFrame ]->verts[ numVerts + 0 ].color.rgba, backend.color2D );
		VectorCopy4( backendData[ rg.smpFrame ]->verts[ numVerts + 1 ].color.rgba, backend.color2D );
		VectorCopy4( backendData[ rg.smpFrame ]->verts[ numVerts + 2 ].color.rgba, backend.color2D );
		VectorCopy4( backendData[ rg.smpFrame ]->verts[ numVerts + 3 ].color.rgba, backend.color2D );

//		R_VaoPackColor( backendData[ rg.smpFrame ]->verts[ numVerts + 0 ].color.rgba, backend.color2D );
//		R_VaoPackColor( backendData[ rg.smpFrame ]->verts[ numVerts + 1 ].color.rgba, backend.color2D );
//		R_VaoPackColor( backendData[ rg.smpFrame ]->verts[ numVerts + 2 ].color.rgba, backend.color2D );
//		R_VaoPackColor( backendData[ rg.smpFrame ]->verts[ numVerts + 3 ].color.rgba, backend.color2D );
	}

	backendData[ rg.smpFrame ]->verts[ numVerts + 0 ].xyz[0] = cmd->x;
	backendData[ rg.smpFrame ]->verts[ numVerts + 0 ].xyz[1] = cmd->y;
	backendData[ rg.smpFrame ]->verts[ numVerts + 0 ].xyz[2] = 0;

	backendData[ rg.smpFrame ]->verts[ numVerts + 0 ].st[0] = cmd->u1;
	backendData[ rg.smpFrame ]->verts[ numVerts + 0 ].st[1] = cmd->v1;

	backendData[ rg.smpFrame ]->verts[ numVerts + 1 ].xyz[0] = cmd->x + cmd->w;
	backendData[ rg.smpFrame ]->verts[ numVerts + 1 ].xyz[1] = cmd->y;
	backendData[ rg.smpFrame ]->verts[ numVerts + 1 ].xyz[2] = 0;

	backendData[ rg.smpFrame ]->verts[ numVerts + 1 ].st[0] = cmd->u2;
	backendData[ rg.smpFrame ]->verts[ numVerts + 1 ].st[1] = cmd->v1;

	backendData[ rg.smpFrame ]->verts[ numVerts + 2 ].xyz[0] = cmd->x + cmd->w;
	backendData[ rg.smpFrame ]->verts[ numVerts + 2 ].xyz[1] = cmd->y + cmd->h;
	backendData[ rg.smpFrame ]->verts[ numVerts + 2 ].xyz[2] = 0;

	backendData[ rg.smpFrame ]->verts[ numVerts + 2 ].st[0] = cmd->u2;
	backendData[ rg.smpFrame ]->verts[ numVerts + 2 ].st[1] = cmd->v2;

	backendData[ rg.smpFrame ]->verts[ numVerts + 3 ].xyz[0] = cmd->x;
	backendData[ rg.smpFrame ]->verts[ numVerts + 3 ].xyz[1] = cmd->y + cmd->h;
	backendData[ rg.smpFrame ]->verts[ numVerts + 3 ].xyz[2] = 0;

	backendData[ rg.smpFrame ]->verts[ numVerts + 3 ].st[0] = cmd->u1;
	backendData[ rg.smpFrame ]->verts[ numVerts + 3 ].st[1] = cmd->v2;

	backendData[ rg.smpFrame ]->indices[ numIndices + 0 ] = 0;
	backendData[ rg.smpFrame ]->indices[ numIndices + 1 ] = 1;
	backendData[ rg.smpFrame ]->indices[ numIndices + 2 ] = 2;
	backendData[ rg.smpFrame ]->indices[ numIndices + 3 ] = 0;
	backendData[ rg.smpFrame ]->indices[ numIndices + 4 ] = 2;
	backendData[ rg.smpFrame ]->indices[ numIndices + 5 ] = 3;

	return (const void *)( cmd + 1 );
}

static const void *RB_DrawWorldView( const void *data )
{
	const drawWorldView_t *cmd;
	
	cmd = (const drawWorldView_t *)data;

	// draw the tilemap
	R_DrawWorld();

	RE_ProcessEntities();

	// render all submitted sgame polygons
	R_DrawPolys();

	return (const void *)( cmd + 1 );
}

void RB_ExecuteRenderCommands( const void *data )
{
	uint64_t t1, t2;

	t1 = ri.Milliseconds();

	while ( 1 ) {
		data = PADP( data, sizeof( void * ) );

		switch ( *(const renderCmdType_t *)data ) {
		case RC_SET_COLOR:
			data = RB_SetColor( data );
			break;
		case RC_DRAW_IMAGE:
			data = RB_DrawImage( data );
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data );
			break;
		case RC_SCREENSHOT:
			backendData[ rg.smpFrame ]->screenshotBuf = *(const screenshotCommand_t *)data;
			data = (const void *)( (const screenshotCommand_t *)data + 1 );
			break;
		case RC_COLORMASK:
			data = RB_ColorMask( data );
			break;
		case RC_CLEARDEPTH:
			data = RB_ClearDepth( data );
			break;
		case RC_POSTPROCESS:
			data = RB_PostProcess( data );
			break;
		case RC_DRAW_WORLDVIEW:
			data = RB_DrawWorldView( data );
			break;
		case RC_END_OF_LIST:
		default:
			// finish any drawing if needed
			RB_FlushBatchBuffer();

			// stop rendering
			t2 = ri.Milliseconds();
			backend.pc.msec = t2 - t1;
			return;
		}
	}
}