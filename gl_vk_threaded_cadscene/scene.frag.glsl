/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#version 440 core
/**/

//#extension GL_ARB_shading_language_include : enable
#include "common.h"

#ifdef VULKAN

  #if UNIFORMS_TECHNIQUE == UNIFORMS_MULTISETSDYNAMIC || UNIFORMS_TECHNIQUE == UNIFORMS_MULTISETSSTATIC
  
    layout(set=DRAW_UBO_SCENE, binding=0, std140) uniform sceneBuffer {
      SceneData       scene;
    };
    layout(set=DRAW_UBO_MATERIAL, binding=0, std140) uniform materialBuffer {
      MaterialData    material;
    };
    
  #elif UNIFORMS_TECHNIQUE == UNIFORMS_ALLDYNAMIC || UNIFORMS_TECHNIQUE == UNIFORMS_SPLITDYNAMIC
  
    layout(set=0, binding=DRAW_UBO_SCENE, std140) uniform sceneBuffer {
      SceneData       scene;
    };
    layout(set=0, binding=DRAW_UBO_MATERIAL, std140) uniform materialBuffer {
      MaterialData    material;
    };
    
  #elif UNIFORMS_TECHNIQUE == UNIFORMS_PUSHCONSTANTS_RAW
  
    layout(set=0, binding=DRAW_UBO_SCENE, std140) uniform sceneBuffer {
      SceneData       scene;
    };
    layout(std140, push_constant) uniform materialBuffer {
      layout(offset = 128)
      MaterialData    material;
    };
    
  #elif UNIFORMS_TECHNIQUE == UNIFORMS_PUSHCONSTANTS_INDEX
  
    #define USE_INDEXING 1
  
    layout(std140, push_constant) uniform indexSetup {
      int matrixIndex;
      int materialIndex;
    };  
    layout(set=0, binding=DRAW_UBO_SCENE, std140) uniform sceneBuffer {
      SceneData   scene;
    };
    layout(set=0, binding=DRAW_UBO_MATERIAL, std430) readonly buffer materialBuffer {
      MaterialData    materials[];
    };
    
  #endif

#else
  layout(binding=DRAW_UBO_SCENE, std140) uniform sceneBuffer {
    SceneData       scene;
  };
  layout(binding=DRAW_UBO_MATERIAL, std140) uniform materialBuffer {
    MaterialData    material;
  };
#endif

layout(location=0) in Interpolants {
  vec3 wPos;
  vec3 wNormal;
} IN;

layout(location=0,index=0) out vec4 out_Color;

void main()
{

#if USE_INDEXING

  int mi = materialIndex * 2; // due to ubo-256 byte padding * 2, FIXME should use cleaner approach
  
  if (WIREMODE != 0){
    out_Color = materials[mi].sides[1].diffuse*1.5 + 0.3;
  }
  else {
    MaterialSide side = materials[mi].sides[gl_FrontFacing ? 1 : 0];
    
#else

  if (WIREMODE != 0){
    out_Color = material.sides[1].diffuse*1.5 + 0.3;
  }
  else {
    MaterialSide side = material.sides[gl_FrontFacing ? 1 : 0];
    
#endif

    vec4 color = side.ambient + side.emissive;
  
    vec3 eyePos = vec3(scene.viewMatrixIT[0].w,scene.viewMatrixIT[1].w,scene.viewMatrixIT[2].w);

    vec3 lightDir = normalize( scene.wLightPos.xyz - IN.wPos);
    vec3 viewDir  = normalize( eyePos - IN.wPos);
    vec3 halfDir  = normalize(lightDir + viewDir);
    vec3 normal   = normalize(IN.wNormal) * (gl_FrontFacing ? 1 : -1);
  
    float ldot = dot(normal,lightDir);
    normal *= sign(ldot);
    ldot   *= sign(ldot);
  
    color += side.diffuse * ldot;
    color += side.specular * pow(max(0,dot(normal,halfDir)),16);

    out_Color = color;
  }
}
