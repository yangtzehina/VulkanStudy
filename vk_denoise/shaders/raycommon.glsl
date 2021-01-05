struct PerRayData_pathtrace
{
  vec3 result;
  vec3 radiance;
  vec3 attenuation;
  vec3 origin;
  vec3 direction;
  vec3 normal;
  vec3 albedo;
  uint seed;
  int  depth;
  int  done;
};

struct PerRayData_pick
{
  vec4 worldPos;
  vec4 barycentrics;
  uint instanceID;
  uint primitiveID;
};


struct Light
{
  vec4 position;
  vec4 color;
  //  float intensity;
  //  float _pad;
};

// Per Instance information
struct primInfo
{
  uint indexOffset;
  uint vertexOffset;
  uint materialIndex;
};

// Matrices buffer for all instances
struct InstancesMatrices
{
  mat4 world;
  mat4 worldIT;
};

struct Scene
{
  mat4  projection;
  mat4  model;
  vec4  camPos;
  int   nbLights;  // w = lightRadiance
  int   _pad1;
  int   _pad2;
  int   _pad3;
  Light lights[10];
};

struct Material
{
  int shadingModel;  // 0: metallic-roughness, 1: specular-glossiness

  // PbrMetallicRoughness
  vec4  pbrBaseColorFactor;
  int   pbrBaseColorTexture;
  float pbrMetallicFactor;
  float pbrRoughnessFactor;
  int   pbrMetallicRoughnessTexture;

  // KHR_materials_pbrSpecularGlossiness
  vec4  khrDiffuseFactor;
  int   khrDiffuseTexture;
  vec3  khrSpecularFactor;
  float khrGlossinessFactor;
  int   khrSpecularGlossinessTexture;

  int   emissiveTexture;
  vec3  emissiveFactor;
  int   alphaMode;
  float alphaCutoff;
  bool  doubleSided;

  int   normalTexture;
  float normalTextureScale;
  int   occlusionTexture;
  float occlusionTextureStrength;
};
