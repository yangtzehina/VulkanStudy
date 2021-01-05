/* Copyright (c) 2017-2018, NVIDIA CORPORATION. All rights reserved.
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


#pragma once

#include "cadscene.hpp"

#include <nvvk/buffers_vk.hpp>
#include <nvvk/commands_vk.hpp>
#include <nvvk/memorymanagement_vk.hpp>

// ScopeStaging handles uploads and other staging operations.
// not efficient because it blocks/syncs operations

struct ScopeStaging
{

  ScopeStaging(VkDevice device, VkPhysicalDevice physical, VkQueue queue_, uint32_t queueFamily, VkDeviceSize size = 128 * 1024 * 1024)
      : staging(device, physical, size)
      , cmdPool(device, queueFamily)
      , queue(queue_)
      , cmd(VK_NULL_HANDLE)
  {
    staging.setFreeUnusedOnRelease(false);
  }

  VkCommandBuffer            cmd;
  nvvk::StagingMemoryManager staging;
  nvvk::CommandPool          cmdPool;
  VkQueue                    queue;

  VkCommandBuffer getCmd()
  {
    cmd = cmd ? cmd : cmdPool.createCommandBuffer();
    return cmd;
  }

  void submit()
  {
    if(cmd)
    {
      cmdPool.submitAndWait(cmd, queue);
      cmd = VK_NULL_HANDLE;
    }
  }

  void upload(const VkDescriptorBufferInfo& binding, const void* data)
  {
    if(cmd && (data == nullptr || !staging.fitsInAllocated(binding.range)))
    {
      submit();
      staging.releaseResources();
    }
    if(data && binding.range)
    {
      staging.cmdToBuffer(getCmd(), binding.buffer, binding.offset, binding.range, data);
    }
  }
};


// GeometryMemoryVK manages vbo/ibo etc. in chunks
// allows to reduce number of bindings and be more memory efficient

struct GeometryMemoryVK
{
  typedef size_t Index;


  struct Allocation
  {
    Index        chunkIndex;
    VkDeviceSize vboOffset;
    VkDeviceSize iboOffset;
  };

  struct Chunk
  {
    VkBuffer vbo;
    VkBuffer ibo;

    VkDeviceSize vboSize;
    VkDeviceSize iboSize;

    nvvk::AllocationID vboAID;
    nvvk::AllocationID iboAID;
  };


  VkDevice                     m_device = VK_NULL_HANDLE;
  nvvk::DeviceMemoryAllocator* m_memoryAllocator;
  std::vector<Chunk>           m_chunks;

  void init(VkDevice device, VkPhysicalDevice physicalDevice, nvvk::DeviceMemoryAllocator* deviceAllocator, VkDeviceSize vboStride, VkDeviceSize maxChunk);
  void deinit();
  void alloc(VkDeviceSize vboSize, VkDeviceSize iboSize, Allocation& allocation);
  void finalize();

  const Chunk& getChunk(const Allocation& allocation) const { return m_chunks[allocation.chunkIndex]; }

  const Chunk& getChunk(Index index) const { return m_chunks[index]; }

  VkDeviceSize getVertexSize() const
  {
    VkDeviceSize size = 0;
    for(size_t i = 0; i < m_chunks.size(); i++)
    {
      size += m_chunks[i].vboSize;
    }
    return size;
  }

  VkDeviceSize getIndexSize() const
  {
    VkDeviceSize size = 0;
    for(size_t i = 0; i < m_chunks.size(); i++)
    {
      size += m_chunks[i].iboSize;
    }
    return size;
  }

  VkDeviceSize getChunkCount() const { return m_chunks.size(); }

private:
  VkDeviceSize m_alignment;
  VkDeviceSize m_vboAlignment;
  VkDeviceSize m_maxVboChunk;
  VkDeviceSize m_maxIboChunk;

  Index getActiveIndex() { return (m_chunks.size() - 1); }

  Chunk& getActiveChunk()
  {
    assert(!m_chunks.empty());
    return m_chunks[getActiveIndex()];
  }
};


class CadSceneVK
{
public:
  struct Geometry
  {
    GeometryMemoryVK::Allocation allocation;

    VkDescriptorBufferInfo vbo;
    VkDescriptorBufferInfo ibo;
  };

  struct Buffers
  {
    VkBuffer materials    = VK_NULL_HANDLE;
    VkBuffer matrices     = VK_NULL_HANDLE;
    VkBuffer matricesOrig = VK_NULL_HANDLE;

    nvvk::AllocationID materialsAID;
    nvvk::AllocationID matricesAID;
    nvvk::AllocationID matricesOrigAID;
  };

  struct Infos
  {
    VkDescriptorBufferInfo materialsSingle;
    VkDescriptorBufferInfo materials;
    VkDescriptorBufferInfo matricesSingle;
    VkDescriptorBufferInfo matrices;
    VkDescriptorBufferInfo matricesOrig;
  };


  VkDevice                    m_device = VK_NULL_HANDLE;
  nvvk::DeviceMemoryAllocator m_memAllocator;

  Buffers m_buffers;
  Infos   m_infos;

  std::vector<Geometry> m_geometry;
  GeometryMemoryVK      m_geometryMem;


  void init(const CadScene& cadscene, VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, uint32_t queueFamilyIndex);
  void deinit();
};
