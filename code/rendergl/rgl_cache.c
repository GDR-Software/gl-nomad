#include "rgl_local.h"

void R_VaoPackTangent(int16_t *out, vec4_t v)
{
	out[0] = v[0] * 32767.0f + (v[0] > 0.0f ? 0.5f : -0.5f);
	out[1] = v[1] * 32767.0f + (v[1] > 0.0f ? 0.5f : -0.5f);
	out[2] = v[2] * 32767.0f + (v[2] > 0.0f ? 0.5f : -0.5f);
	out[3] = v[3] * 32767.0f + (v[3] > 0.0f ? 0.5f : -0.5f);
}

void R_VaoPackNormal(int16_t *out, vec3_t v)
{
	out[0] = v[0] * 32767.0f + (v[0] > 0.0f ? 0.5f : -0.5f);
	out[1] = v[1] * 32767.0f + (v[1] > 0.0f ? 0.5f : -0.5f);
	out[2] = v[2] * 32767.0f + (v[2] > 0.0f ? 0.5f : -0.5f);
	out[3] = 0;
}

void R_VaoPackColor(uint16_t *out, const vec4_t c)
{
	out[0] = c[0] * 65535.0f + 0.5f;
	out[1] = c[1] * 65535.0f + 0.5f;
	out[2] = c[2] * 65535.0f + 0.5f;
	out[3] = c[3] * 65535.0f + 0.5f;
}

void R_VaoUnpackTangent(vec4_t v, int16_t *pack)
{
	v[0] = pack[0] / 32767.0f;
	v[1] = pack[1] / 32767.0f;
	v[2] = pack[2] / 32767.0f;
	v[3] = pack[3] / 32767.0f;
}

void R_VaoUnpackNormal(vec3_t v, int16_t *pack)
{
	v[0] = pack[0] / 32767.0f;
	v[1] = pack[1] / 32767.0f;
	v[2] = pack[2] / 32767.0f;
}

static vertexBuffer_t *hashTable[MAX_RENDER_BUFFERS];

static qboolean R_BufferExists(const char *name)
{
    return hashTable[Com_GenerateHashValue(name, MAX_RENDER_BUFFERS)] != NULL;
}

static void R_SetVertexPointers(const vertexAttrib_t attribs[ATTRIB_INDEX_COUNT])
{
	uint32_t attribBit;
	const vertexAttrib_t *vAtb;

    for (uint64_t i = 0; i < ATTRIB_INDEX_COUNT; i++) {
		attribBit = 1 << i;
		vAtb = &attribs[i];

        if (vAtb->enabled) {
            nglVertexAttribPointer(vAtb->index, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, (const void *)vAtb->offset);
			if (!(glState.vertexAttribsEnabled & attribBit))
				nglEnableVertexAttribArray(vAtb->index);
			
			glState.vertexAttribsEnabled |= attribBit;
        }
        else {
			if ((glState.vertexAttribsEnabled & attribBit))
	            nglDisableVertexAttribArray(vAtb->index);
			
			glState.vertexAttribsEnabled &= ~attribBit;
        }
    }
}

void VBO_SetVertexPointers(vertexBuffer_t *vbo, uint32_t attribBits)
{
	// if nothing is set, set everything
	if (!(attribBits & ATTRIB_BITS))
		attribBits = ATTRIB_BITS;
	
	if (attribBits & ATTRIB_POSITION) {
		vbo->attribs[ATTRIB_INDEX_POSITION].enabled = qtrue;
	}
	if (attribBits & ATTRIB_TEXCOORD) {
		vbo->attribs[ATTRIB_INDEX_TEXCOORD].enabled = qtrue;
	}
	if (attribBits & ATTRIB_COLOR) {
		vbo->attribs[ATTRIB_INDEX_COLOR].enabled = qtrue;
	}

	R_SetVertexPointers(vbo->attribs);
}

/*
R_ClearVertexPointers: clears all vertex pointers in the current GL state
*/
static void R_ClearVertexPointers(void)
{
	uint32_t attribBit;
    for (uint32_t i = 0; i < ATTRIB_INDEX_COUNT; i++) {
		attribBit = 1 << i;
		if ((glState.vertexAttribsEnabled & attribBit))
	        nglDisableVertexAttribArray(i);
    }
	glState.vertexAttribsEnabled = 0;
}

void R_InitGPUBuffers(void)
{
	uint64_t verticesSize, indicesSize;
	uintptr_t offset;

	ri.Printf(PRINT_INFO, "---------- R_InitGPUBuffers ----------\n");

	rg.numBuffers = 0;

	backend.drawBuffer = R_AllocateBuffer( "batchBuffer", NULL, sizeof(polyVert_t) * MAX_BATCH_VERTICES, NULL, sizeof(glIndex_t) * MAX_BATCH_INDICES, BUFFER_FRAME );

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].enabled		= qtrue;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].enabled		= qtrue;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].enabled			= qtrue;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].enabled		= qfalse;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].count		= 3;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].count		= 2;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].count			= 4;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].count			= 4;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].type			= GL_FLOAT;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].type			= GL_FLOAT;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].type			= GL_UNSIGNED_BYTE;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].type			= GL_SHORT;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].index		= ATTRIB_INDEX_POSITION;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].index		= ATTRIB_INDEX_TEXCOORD;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].index			= ATTRIB_INDEX_COLOR;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].index			= ATTRIB_INDEX_NORMAL;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].normalized	= GL_FALSE;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].normalized	= GL_FALSE;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].normalized		= GL_TRUE;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].normalized		= GL_TRUE;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].offset		= offsetof( polyVert_t, xyz );
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].offset		= offsetof( polyVert_t, uv );
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].offset			= offsetof( polyVert_t, modulate );
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].offset			= 0;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].stride		= sizeof(polyVert_t);
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].stride		= sizeof(polyVert_t);
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].stride			= sizeof(polyVert_t);
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].stride			= sizeof(polyVert_t);

	VBO_BindNull();

	GL_CheckErrors();
}

void R_ShutdownGPUBuffers(void)
{
	uint64_t i;
	vertexBuffer_t *vbo;

	ri.Printf(PRINT_INFO, "---------- R_ShutdownGPUBuffers -----------\n");

	VBO_BindNull();

	for (i = 0; i < rg.numBuffers; i++) {
		vbo = rg.buffers[i];

		R_ShutdownBuffer( vbo );
	}

	memset( rg.buffers, 0, sizeof(rg.buffers) );
	memset( hashTable, 0, sizeof(hashTable) );
	rg.numBuffers = 0;
}

static void R_InitGPUMemory(GLenum target, GLuint id, void *data, uint32_t size, bufferType_t usage)
{
	GLbitfield bits;
	GLenum glUsage;

	switch (usage) {
	case BUFFER_DYNAMIC:
	case BUFFER_FRAME:
		glUsage = GL_DYNAMIC_DRAW;
		break;
	case BUFFER_STATIC:
		glUsage = GL_STATIC_DRAW;
		break;
	default:
		ri.Error(ERR_FATAL, "R_AllocateBuffer: invalid buffer usage %i", usage);
	};

#if 0
	// zero clue how well this'll work
	if (r_experimental->i) {
		if (glContext.ARB_map_buffer_range) {
			bits = GL_MAP_READ_BIT;

			if (glUsage == GL_DYNAMIC_DRAW) {
				bits |= GL_MAP_WRITE_BIT;
				if (usage == BUFFER_FRAME) {
					bits |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
				}
			}

			if (glContext.ARB_buffer_storage || NGL_VERSION_ATLEAST(4, 5)) {
				nglBufferStorage(target, size, data, bits | (glUsage == GL_DYNAMIC_DRAW ? GL_DYNAMIC_STORAGE_BIT : 0));
			}
			else {
				nglBufferData(target, size, NULL, glUsage);
			}
		}
	}
	else {
		nglBufferData(target, size, data, glUsage);
	}
#endif
}

vertexBuffer_t *R_AllocateBuffer(const char *name, void *vertices, uint32_t verticesSize, void *indices, uint32_t indicesSize, bufferType_t type)
{
    vertexBuffer_t *buf;
    uint64_t hash;
	GLenum usage;
	uint32_t namelen;

	switch (type) {
	case BUFFER_STATIC:
		usage = GL_STATIC_DRAW;
		break;
	case BUFFER_FRAME:
	case BUFFER_DYNAMIC:
		usage = GL_DYNAMIC_DRAW;
		break;
	case BUFFER_STREAM:
		usage = GL_STREAM_DRAW;
		break;
	default:
		ri.Error(ERR_FATAL, "Bad glUsage %i", type);
	};

	if (R_BufferExists( name )) {
		ri.Error( ERR_DROP, "R_AllocateBuffer: buffer '%s' already exists", name );
	}
	if (rg.numBuffers == MAX_RENDER_BUFFERS) {
		ri.Error(ERR_DROP, "R_AllocateBuffer: MAX_RENDER_BUFFERS hit");
	}

    hash = Com_GenerateHashValue(name, MAX_RENDER_BUFFERS);

    // these buffers are only allocated on a per vm
    // and map basis, so use the hunk
    buf = rg.buffers[rg.numBuffers] = ri.Hunk_Alloc(sizeof(*buf), h_low);
    hashTable[hash] = buf;
    rg.numBuffers++;

    buf->type = type;

	nglGenVertexArrays(1, (GLuint *)&buf->vaoId);
	nglBindVertexArray(buf->vaoId);

	buf->vertex.usage = BUF_GL_BUFFER;
	buf->index.usage = BUF_GL_BUFFER;

	buf->vertex.glUsage = usage;
	buf->index.glUsage = usage;

	buf->vertex.size = verticesSize;
	buf->index.size = indicesSize;

	nglGenBuffers(1, (GLuint *)&buf->vertex.id);
	nglBindBuffer(GL_ARRAY_BUFFER, buf->vertex.id);
	nglBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, usage);

	nglGenBuffers(1, (GLuint *)&buf->index.id);
	nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->index.id);
	nglBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, indices, usage);

	GL_SetObjectDebugName(GL_VERTEX_ARRAY, buf->vaoId, name, "_vao");
	GL_SetObjectDebugName(GL_BUFFER, buf->vertex.id, name, "_vbo");
	GL_SetObjectDebugName(GL_BUFFER, buf->index.id, name, "_ibo");

    return buf;
}

/*
============
VBO_Bind
============
*/
void VBO_Bind(vertexBuffer_t *vbo)
{
	if (!vbo) {
		ri.Error(ERR_DROP, "VBO_Bind: NULL buffer");
		return;
	}

	if (glState.currentVao != vbo) {
		glState.currentVao = vbo;
		glState.vaoId = vbo->vaoId;
		glState.vboId = vbo->vertex.id;
		glState.iboId = vbo->index.id;
		backend.pc.c_bufferBinds++;

		nglBindVertexArray(vbo->vaoId);
		nglBindBuffer( GL_ARRAY_BUFFER, vbo->vertex.id );
		nglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo->index.id );

		// Intel Graphics doesn't save GL_ELEMENT_ARRAY_BUFFER binding with VAO binding.
		if (glContext.intelGraphics)
			nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->index.id);
	}
}

/*
============
VBO_BindNull
============
*/
void VBO_BindNull(void)
{
	ri.GLimp_LogComment("--- VBO_BindNull ---\n");

	if (glState.currentVao) {
		glState.currentVao = NULL;
		glState.vaoId = glState.vboId = glState.iboId = 0;
        nglBindVertexArray(0);
		nglBindBuffer( GL_ARRAY_BUFFER, 0 );
		nglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        // why you no save GL_ELEMENT_ARRAY_BUFFER binding, Intel?
        if (glContext.intelGraphics) nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	GL_CheckErrors();
}

void R_ShutdownBuffer( vertexBuffer_t *vbo )
{
	if (vbo->vaoId)
		nglDeleteVertexArrays(1, (const GLuint *)&vbo->vaoId);

	if (vbo->vertex.id) {
		if (vbo->vertex.usage == BUF_GL_MAPPED) {
			nglBindBuffer(GL_ARRAY_BUFFER, vbo->vertex.id);
			nglUnmapBuffer(GL_ARRAY_BUFFER);
			nglBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		nglDeleteBuffers(1, (const GLuint *)&vbo->vertex.id);
	}
	
	if (vbo->index.id) {
		if (vbo->index.usage == BUF_GL_MAPPED) {
			nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->index.id);
			nglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
			nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		nglDeleteBuffers(1, (const GLuint *)&vbo->index.id);
	}
}



void RB_SetBatchBuffer( vertexBuffer_t *buffer, void *vertexBuffer, uintptr_t vtxSize, void *indexBuffer, uintptr_t idxSize )
{
    // is it already bound?
    if (backend.drawBatch.buffer == buffer) {
        return;
    }
	else if (backend.drawBatch.buffer != buffer) {
		VBO_BindNull();
	}

    // clear anything currently queued
    if (backend.drawBatch.buffer && (backend.drawBatch.vtxOffset || backend.drawBatch.idxOffset)) {
        RB_FlushBatchBuffer();
    }

    backend.drawBatch.buffer = buffer;

    backend.drawBatch.vtxOffset = 0;
    backend.drawBatch.idxOffset = 0;

    backend.drawBatch.vtxDataSize = vtxSize;
    backend.drawBatch.idxDataSize = idxSize;

    backend.drawBatch.maxVertices = buffer->vertex.size;
    backend.drawBatch.maxIndices = buffer->index.size;

    backend.drawBatch.vertices = vertexBuffer;
    backend.drawBatch.indices = indexBuffer;

    // bind the new cache
    VBO_Bind( buffer );

	R_ClearVertexPointers();

    // set the new vertex attrib array state
	R_SetVertexPointers( buffer->attribs );
}

void RB_FlushBatchBuffer( void )
{
    if (!backend.drawBatch.buffer) {
        return;
    }

    // do we actually have something there?
    if (backend.drawBatch.vtxOffset == 0 && backend.drawBatch.idxOffset == 0) {
        return;
    }

    backend.pc.c_dynamicBufferDraws++;

    //
    // upload the data to the gpu
    //

    // orphan the old vertex buffer so that we don't stall on it
    nglBufferData( GL_ARRAY_BUFFER, backend.drawBatch.maxVertices, NULL, backend.drawBatch.buffer->vertex.glUsage );
    nglBufferSubData( GL_ARRAY_BUFFER, 0, backend.drawBatch.vtxOffset * backend.drawBatch.vtxDataSize, backend.drawBatch.vertices );

    // orphan the old index buffer so that we don't stall on it
    nglBufferData( GL_ELEMENT_ARRAY_BUFFER, backend.drawBatch.maxIndices, NULL, backend.drawBatch.buffer->index.glUsage );
    nglBufferSubData( GL_ELEMENT_ARRAY_BUFFER, 0, backend.drawBatch.idxOffset * backend.drawBatch.idxDataSize, backend.drawBatch.indices );

    R_DrawElements( backend.drawBatch.idxOffset, 0 );

	backend.pc.c_bufferIndices += backend.drawBatch.idxOffset;
	backend.pc.c_bufferVertices += backend.drawBatch.vtxOffset;

    backend.drawBatch.idxOffset = 0;
    backend.drawBatch.vtxOffset = 0;
}

void RB_CommitDrawData( const void *verts, uint32_t numVerts, const void *indices, uint32_t numIndices )
{
    if (numVerts >= backend.drawBatch.maxVertices) {
        ri.Error( ERR_DROP, "RB_CommitDrawData: numVerts >= backend.drawBatch.maxVertices (%i >= %i)", numVerts, backend.drawBatch.maxVertices );
    }
    if (numIndices >= backend.drawBatch.maxIndices) {
        ri.Error( ERR_DROP, "RB_CommitDrawData: numIndices >= backend.drawBatch.maxIndices (%i >= %i)", numIndices, backend.drawBatch.maxIndices );
    }

    // do we need to flush?
    if (backend.drawBatch.vtxOffset + numVerts >= backend.drawBatch.maxVertices
    || backend.drawBatch.idxOffset + numIndices >= backend.drawBatch.maxIndices) {
        RB_FlushBatchBuffer();
    }

    //
    // copy the data into the client side buffer
    //

    // we could be submitting either indices or vertices
    if (verts) {
        memcpy( (byte *)backend.drawBatch.vertices + (backend.drawBatch.vtxOffset * backend.drawBatch.vtxDataSize), verts, numVerts * backend.drawBatch.vtxDataSize );
    }
    if (indices) {
        memcpy( (byte *)backend.drawBatch.indices + (backend.drawBatch.idxOffset * backend.drawBatch.idxDataSize), indices, numIndices * backend.drawBatch.idxDataSize );
    }

    backend.drawBatch.vtxOffset += numVerts;
    backend.drawBatch.idxOffset += numIndices;
}
