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

/* Contact ckubisch@nvidia.com (Christoph Kubisch) for feedback */


#ifndef CADSCENE_H__
#define CADSCENE_H__

#include <nvmath/nvmath.h>
#include <vector>
#include <cstdint>

class CadScene
{

public:
  struct BBox
  {
    nvmath::vec4f min;
    nvmath::vec4f max;

    BBox()
        : min(FLT_MAX)
        , max(-FLT_MAX)
    {
    }

    inline void merge(const nvmath::vec4f& point)
    {
      min = nvmath::nv_min(min, point);
      max = nvmath::nv_max(max, point);
    }

    inline void merge(const BBox& bbox)
    {
      min = nvmath::nv_min(min, bbox.min);
      max = nvmath::nv_max(max, bbox.max);
    }

    inline BBox transformed(const nvmath::mat4f& matrix, int dim = 3)
    {
      int           i;
      nvmath::vec4f box[16];
      // create box corners
      box[0] = nvmath::vec4f(min.x, min.y, min.z, min.w);
      box[1] = nvmath::vec4f(max.x, min.y, min.z, min.w);
      box[2] = nvmath::vec4f(min.x, max.y, min.z, min.w);
      box[3] = nvmath::vec4f(max.x, max.y, min.z, min.w);
      box[4] = nvmath::vec4f(min.x, min.y, max.z, min.w);
      box[5] = nvmath::vec4f(max.x, min.y, max.z, min.w);
      box[6] = nvmath::vec4f(min.x, max.y, max.z, min.w);
      box[7] = nvmath::vec4f(max.x, max.y, max.z, min.w);

      box[8]  = nvmath::vec4f(min.x, min.y, min.z, max.w);
      box[9]  = nvmath::vec4f(max.x, min.y, min.z, max.w);
      box[10] = nvmath::vec4f(min.x, max.y, min.z, max.w);
      box[11] = nvmath::vec4f(max.x, max.y, min.z, max.w);
      box[12] = nvmath::vec4f(min.x, min.y, max.z, max.w);
      box[13] = nvmath::vec4f(max.x, min.y, max.z, max.w);
      box[14] = nvmath::vec4f(min.x, max.y, max.z, max.w);
      box[15] = nvmath::vec4f(max.x, max.y, max.z, max.w);

      // transform box corners
      // and find new mins,maxs
      BBox bbox;

      for(i = 0; i < (1 << dim); i++)
      {
        nvmath::vec4f point = matrix * box[i];
        bbox.merge(point);
      }

      return bbox;
    }
  };

  struct MaterialSide
  {
    nvmath::vec4f ambient;
    nvmath::vec4f diffuse;
    nvmath::vec4f specular;
    nvmath::vec4f emissive;
  };

  // need to keep this 256 byte aligned (UBO range)
  struct Material
  {
    MaterialSide sides[2];
    unsigned int _pad[32];

    Material() { memset(this, 0, sizeof(Material)); }
  };

  // need to keep this 256 byte aligned (UBO range)
  struct MatrixNode
  {
    nvmath::mat4f worldMatrix;
    nvmath::mat4f worldMatrixIT;
    nvmath::mat4f objectMatrix;
    nvmath::mat4f objectMatrixIT;
  };

  struct Vertex
  {
    nvmath::vec3f position;
    uint16_t      normalOctX;
    uint16_t      normalOctY;
  };

  struct DrawRange
  {
    size_t offset;
    int    count;

    DrawRange()
        : offset(0)
        , count(0)
    {
    }
  };

  struct DrawStateInfo
  {
    int materialIndex;
    int matrixIndex;

    friend bool operator!=(const DrawStateInfo& lhs, const DrawStateInfo& rhs)
    {
      return lhs.materialIndex != rhs.materialIndex || lhs.matrixIndex != rhs.matrixIndex;
    }

    friend bool operator==(const DrawStateInfo& lhs, const DrawStateInfo& rhs)
    {
      return lhs.materialIndex == rhs.materialIndex && lhs.matrixIndex == rhs.matrixIndex;
    }
  };

  struct DrawRangeCache
  {
    std::vector<DrawStateInfo> state;
    std::vector<int>           stateCount;

    std::vector<size_t> offsets;
    std::vector<int>    counts;
  };

  struct GeometryPart
  {
    DrawRange indexSolid;
    DrawRange indexWire;
  };

  struct Geometry
  {
    int    cloneIdx;
    size_t vboSize;
    size_t iboSize;

    Vertex*       vboData;
    unsigned int* iboData;

    std::vector<GeometryPart> parts;

    int numVertices;
    int numIndexSolid;
    int numIndexWire;
  };

  struct ObjectPart
  {
    int active;
    int materialIndex;
    int matrixIndex;
  };

  struct Object
  {
    int matrixIndex;
    int geometryIndex;

    std::vector<ObjectPart> parts;

    DrawRangeCache cacheSolid;
    DrawRangeCache cacheWire;
  };

  std::vector<Material>      m_materials;
  std::vector<BBox>          m_geometryBboxes;
  std::vector<Geometry>      m_geometry;
  std::vector<MatrixNode>    m_matrices;
  std::vector<Object>        m_objects;
  std::vector<nvmath::vec2i> m_objectAssigns;


  BBox m_bbox;


  void updateObjectDrawCache(Object& object);

  bool loadCSF(const char* filename, int clones = 0, int cloneaxis = 3);
  void unload();
};


#endif
