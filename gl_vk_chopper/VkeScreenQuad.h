/*-----------------------------------------------------------------------
Copyright (c) 2014-2016, NVIDIA. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Neither the name of its contributors may be used to endorse
or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/
/* Contact chebert@nvidia.com (Chris Hebert) for feedback */

#ifndef __H_VKE_SCREEN_QUAD_
#define __H_VKE_SCREEN_QUAD_



#pragma once

#include<nvmath/nvmath.h>
#include"VkeBuffer.h"
#include"VkeVBO.h"
#include"VkeIBO.h"

typedef struct _QuadUniform{
	nvmath::mat4f view_matrix;
}QuadUniform;

class VkeScreenQuad : public VkeBuffer<QuadUniform>
{
public:
	VkeScreenQuad();
	~VkeScreenQuad();

	void bind(VkCommandBuffer *inBuffer);
	void initQuadData();
	void draw(VkCommandBuffer *inBuffer);

	VkeVBO m_vbo;
	VkeIBO m_ibo;

};


#endif