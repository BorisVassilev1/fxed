class ResourceHandle {
	uint index;

	enum ResourceType {
		RESOURCE_TYPE_IMAGE_SAMPLER = 0,
		RESOURCE_TYPE_STORAGE_IMAGE = 1,
		RESOURCE_TYPE_UNIFORM_BUFFER = 2,
		RESOURCE_TYPE_STORAGE_BUFFER = 3,
	};

	uint GetIndex() { return index & 0x1FFFFFFF; }
	bool IsWritable() { return (index & 0x80000000) != 0; }
	ResourceType GetType() { return (ResourceType)((index >> 29) & 0x3); }
	bool IsValid() { return index != 0xFFFFFFFF; }
};

#define BINDLESS_RESCRIPTOR_HEAP_SIZE 100

#define ITERATE_TEXTURE_TYPES(ITERATOR, ...) \
	ITERATOR(int, __VA_ARGS__) \
	ITERATOR(uint, __VA_ARGS__) \
	ITERATOR(float, __VA_ARGS__) \
	ITERATOR(int2, __VA_ARGS__) \
	ITERATOR(uint2, __VA_ARGS__) \
	ITERATOR(float2, __VA_ARGS__) \
	ITERATOR(int3, __VA_ARGS__) \
	ITERATOR(uint3, __VA_ARGS__) \
	ITERATOR(float3, __VA_ARGS__) \
	ITERATOR(int4, __VA_ARGS__) \
	ITERATOR(uint4, __VA_ARGS__) \
	ITERATOR(float4, __VA_ARGS__)

#define ITERATE_SAMPLEABLE_TYPES(ITERATOR, ...) \
	ITERATOR(float, __VA_ARGS__) \
	ITERATOR(float2, __VA_ARGS__) \
	ITERATOR(float3, __VA_ARGS__) \
	ITERATOR(float4, __VA_ARGS__)

#ifdef VULKAN
	#define VK_BINDING_ATTR(a, b) [[vk::binding(a, b)]]
	#define VK_PUSH_CONST_ATTR [[vk::push_constant]]
#else
	#define VK_BINDING_ATTR(a, b)
	#define VK_PUSH_CONST_ATTR
#endif

#define _GENERATE_TEXTURE_TYPE_SLOT(nativeType, textureType, bindingA, bindingB) \
	VK_BINDING_ATTR(bindingA, bindingB) \
	textureType<nativeType> g_##textureType##nativeType [ BINDLESS_RESCRIPTOR_HEAP_SIZE ] : register(t##bindingA);

#define DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(textureType, bindingA, bindingB) \
	ITERATE_TEXTURE_TYPES(_GENERATE_TEXTURE_TYPE_SLOT, textureType, bindingA, bindingB)

#define CONCAT2(a, b) a##b
#define CONCAT3(a, b, c) a##b##c
#define CONCAT4(a, b, c, d) a##b##c##d
#define CONCAT5(a, b, c, d, e) a##b##c##d##e
#define CONCAT(a, b) CONCAT2(a, b)
#define STRINGIFY(a) #a

SamplerState g_Samplers [ BINDLESS_RESCRIPTOR_HEAP_SIZE ] : register(s0);
DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(Texture1D, 0, 0)
DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(Texture2D, 0, 0)
DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS(Texture3D, 0, 0)

class TextureHandle {
	ResourceHandle handle;

	template<typename T>
	T Sample1D(float u);
	template<typename T>
	T Sample2D(float2 uv);
	template<typename T>
	T Sample3D(float3 uvw);

	template<typename T>
	T Sample1D(float u, float lod);
	template<typename T>
	T Sample2D(float2 uv, float lod);
	template<typename T>
	T Sample3D(float3 uvw, float lod);

	template<typename T>
	T Load1D(uint x);
	template<typename T>
	T Load2D(uint2 xy);
	template<typename T>
	T Load3D(uint3 xyz);

	bool IsValid() { return handle.IsValid(); }
};

#define DESCRIPTOR_HEAP(type, index) CONCAT(g_, type [ NonUniformResourceIndex(index) ])

#define DEFINE_1D_OVERLOADS(nativeType, resourceType) \
template<>  \
nativeType TextureHandle::Sample1D<nativeType>(float u) { \
	uint index = handle.GetIndex(); \
	SamplerState sampler = g_Samplers[NonUniformResourceIndex(index)]; \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index).Sample(sampler, u); \
}

#define DEFINE_2D_OVERLOADS(nativeType, resourceType) \
template<>  \
nativeType TextureHandle::Sample2D<nativeType>(float2 uv) { \
	uint index = handle.GetIndex(); \
	SamplerState sampler = g_Samplers[NonUniformResourceIndex(index)]; \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index).Sample(sampler, uv); \
}

#define DEFINE_3D_OVERLOADS(nativeType, resourceType) \
template<>  \
nativeType TextureHandle::Sample3D<nativeType>(float3 uvw) { \
	uint index = handle.GetIndex(); \
	SamplerState sampler = g_Samplers[NonUniformResourceIndex(index)]; \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index).Sample(sampler, uvw); \
}

#define DEFINE_1D_OVERLOADS_LOD(nativeType, resourceType) \
template<>  \
nativeType TextureHandle::Sample1D<nativeType>(float u, float lod) { \
	uint index = handle.GetIndex(); \
	SamplerState sampler = g_Samplers[NonUniformResourceIndex(index)]; \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index).SampleLevel(sampler, u, lod); \
}

#define DEFINE_2D_OVERLOADS_LOD(nativeType, resourceType) \
template<>  \
nativeType TextureHandle::Sample2D<nativeType>(float2 uv, float lod) { \
	uint index = handle.GetIndex(); \
	SamplerState sampler = g_Samplers[NonUniformResourceIndex(index)]; \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index).SampleLevel(sampler, uv, lod); \
}

#define DEFINE_3D_OVERLOADS_LOD(nativeType, resourceType) \
template<>  \
nativeType TextureHandle::Sample3D<nativeType>(float3 uvw, float lod) { \
	uint index = handle.GetIndex(); \
	SamplerState sampler = g_Samplers[NonUniformResourceIndex(index)]; \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index).SampleLevel(sampler, uvw, lod); \
}

#define DEFINE_1D_LOAD_OVERLOADS(nativeType, resourceType) \
template<>  \
nativeType TextureHandle::Load1D<nativeType>(uint x) { \
	uint index = handle.GetIndex(); \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index).Load(uint2(x, 0)); \
}

#define DEFINE_2D_LOAD_OVERLOADS(nativeType, resourceType) \
template<>  \
nativeType TextureHandle::Load2D<nativeType>(uint2 xy) { \
	uint index = handle.GetIndex(); \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index).Load(uint3(xy, 0)); \
}

#define DEFINE_3D_LOAD_OVERLOADS(nativeType, resourceType) \
template<>  \
nativeType TextureHandle::Load3D<nativeType>(uint3 xyz) { \
	uint index = handle.GetIndex(); \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index).Load(uint4(xyz, 0)); \
}

ITERATE_SAMPLEABLE_TYPES(DEFINE_1D_OVERLOADS, Texture1D)
ITERATE_SAMPLEABLE_TYPES(DEFINE_2D_OVERLOADS, Texture2D)
ITERATE_SAMPLEABLE_TYPES(DEFINE_3D_OVERLOADS, Texture3D)
ITERATE_SAMPLEABLE_TYPES(DEFINE_1D_OVERLOADS_LOD, Texture1D)
ITERATE_SAMPLEABLE_TYPES(DEFINE_2D_OVERLOADS_LOD, Texture2D)
ITERATE_SAMPLEABLE_TYPES(DEFINE_3D_OVERLOADS_LOD, Texture3D)
ITERATE_TEXTURE_TYPES(DEFINE_1D_LOAD_OVERLOADS, Texture1D)
ITERATE_TEXTURE_TYPES(DEFINE_2D_LOAD_OVERLOADS, Texture2D)
ITERATE_TEXTURE_TYPES(DEFINE_3D_LOAD_OVERLOADS, Texture3D)


#define _GENERATE_RWTEXTURE_TYPE_SLOT(nativeType, textureType, bindingA, bindingB) \
	VK_BINDING_ATTR(bindingA, bindingB) \
	textureType<nativeType> g_##textureType##nativeType [ BINDLESS_RESCRIPTOR_HEAP_SIZE ] : register(u##bindingA);

#define DEFINE_RWTEXTURE_TYPES_AND_FORMATS_SLOTS(textureType, bindingA, bindingB) \
	ITERATE_TEXTURE_TYPES(_GENERATE_RWTEXTURE_TYPE_SLOT, textureType, bindingA, bindingB)

DEFINE_RWTEXTURE_TYPES_AND_FORMATS_SLOTS(RWTexture1D, 1, 0)
DEFINE_RWTEXTURE_TYPES_AND_FORMATS_SLOTS(RWTexture2D, 1, 0)
DEFINE_RWTEXTURE_TYPES_AND_FORMATS_SLOTS(RWTexture3D, 1, 0)

class RWTextureHandle {
	ResourceHandle handle;

	template<typename T>
	void Store1D(uint x, T value);
	template<typename T>
	void Store2D(uint2 xy, T value);
	template<typename T>
	void Store3D(uint3 xyz, T value);

	template<typename T>
	T Load1D(uint x);
	template<typename T>
	T Load2D(uint2 xy);
	template<typename T>
	T Load3D(uint3 xyz);

	bool IsValid() { return handle.IsValid(); }
};

#define DEFINE_1D_STORE_OVERLOADS(nativeType, resourceType) \
template<>  \
void RWTextureHandle::Store1D<nativeType>(uint x, nativeType value) { \
	uint index = handle.GetIndex(); \
	DESCRIPTOR_HEAP(resourceType##nativeType, index)[x] = value; \
}

#define DEFINE_2D_STORE_OVERLOADS(nativeType, resourceType) \
template<>  \
void RWTextureHandle::Store2D<nativeType>(uint2 xy, nativeType value) { \
	uint index = handle.GetIndex(); \
	DESCRIPTOR_HEAP(resourceType##nativeType, index)[xy] = value; \
}

#define DEFINE_3D_STORE_OVERLOADS(nativeType, resourceType) \
template<>  \
void RWTextureHandle::Store3D<nativeType>(uint3 xyz, nativeType value) { \
	uint index = handle.GetIndex(); \
	DESCRIPTOR_HEAP(resourceType##nativeType, index)[xyz] = value; \
}

ITERATE_TEXTURE_TYPES(DEFINE_1D_STORE_OVERLOADS, RWTexture1D)
ITERATE_TEXTURE_TYPES(DEFINE_2D_STORE_OVERLOADS, RWTexture2D)
ITERATE_TEXTURE_TYPES(DEFINE_3D_STORE_OVERLOADS, RWTexture3D)

#define DEFINE_1D_RWLOAD_OVERLOADS(nativeType, resourceType) \
template<>  \
nativeType RWTextureHandle::Load1D<nativeType>(uint x) { \
	uint index = handle.GetIndex(); \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index)[x];\
}

#define DEFINE_2D_RWLOAD_OVERLOADS(nativeType, resourceType) \
template<>  \
nativeType RWTextureHandle::Load2D<nativeType>(uint2 xy) { \
	uint index = handle.GetIndex(); \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index)[xy]; \
}

#define DEFINE_3D_RWLOAD_OVERLOADS(nativeType, resourceType) \
template<>  \
nativeType RWTextureHandle::Load3D<nativeType>(uint3 xyz) { \
	uint index = handle.GetIndex(); \
	return DESCRIPTOR_HEAP(resourceType##nativeType, index)[xyz]; \
}

ITERATE_TEXTURE_TYPES(DEFINE_1D_RWLOAD_OVERLOADS, RWTexture1D)
ITERATE_TEXTURE_TYPES(DEFINE_2D_RWLOAD_OVERLOADS, RWTexture2D)
ITERATE_TEXTURE_TYPES(DEFINE_3D_RWLOAD_OVERLOADS, RWTexture3D)

#undef _GENERATE_TEXTURE_TYPE_SLOT
#undef DEFINE_TEXTURE_TYPES_AND_FORMATS_SLOTS
#undef ITERATE_TEXTURE_TYPES

ByteAddressBuffer g_StorageBuffer    [ BINDLESS_RESCRIPTOR_HEAP_SIZE ] : register(t3);

class ArrayBufferHandle {
	ResourceHandle handle;

	bool IsValid() { return handle.IsValid(); }

	template<typename T>
	T Load(uint index) {
		uint handleIndex = handle.GetIndex();
		return g_StorageBuffer [ NonUniformResourceIndex(handleIndex) ].Load<T>(index * sizeof(T));
	}
};

RaytracingAccelerationStructure g_AccelerationStructures [ 50 ] : register(t4);

class AccelerationStructureHandle {
	ResourceHandle handle;

	bool IsValid() { return handle.IsValid(); }

	template<typename payload_t>
	void TraceRayKHR(
		uint RayFlags, 
		uint InstanceInclusionMask, 
		uint RayContributionToHitGroupIndex, 
		uint MultiplierForGeometryContributionToHitGroupIndex, 
		uint MissShaderIndex, 
		RayDesc Ray, 
		inout payload_t Payload);
};

template<typename payload_t>
void AccelerationStructureHandle::TraceRayKHR(
	uint RayFlags, 
	uint InstanceInclusionMask, 
	uint RayContributionToHitGroupIndex, 
	uint MultiplierForGeometryContributionToHitGroupIndex, 
	uint MissShaderIndex, 
	RayDesc Ray, 
	inout payload_t Payload) {
	uint handleIndex = handle.GetIndex();
	TraceRay(g_AccelerationStructures [ NonUniformResourceIndex(handleIndex) ], 
		  RayFlags,
		  InstanceInclusionMask, 
		  RayContributionToHitGroupIndex,
		  MultiplierForGeometryContributionToHitGroupIndex, 
		  MissShaderIndex, 
		  Ray, 
		  Payload);
}

