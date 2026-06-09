#include <resource_heap.hlsl>

// Vertex Shader
struct VSInput
{
	float2 position : POSITION;
	float2 texCoord : TEXCOORD0_;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	nointerpolation uint glyphKind : SV_InstanceID;
};

struct PushConstants
{
	int2 viewportSize;
	float2 translation;
	TextureHandle texture;
	float textSize;
	float time;
};

VK_PUSH_CONST_ATTR
PushConstants pushConstants : register(b0);

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
	float2 scale = 2.f * pushConstants.textSize / float2(pushConstants.viewportSize);
	VSOutput output;
	output.position = float4(-1.f + scale * (input.position + pushConstants.translation), 1.0, 1.0);

	output.texCoord = abs(input.texCoord);

	uint signBit0 = asuint(input.texCoord.x) >> 31;
	uint signBit1 = asuint(input.texCoord.y) >> 31;
	output.glyphKind = (signBit1 << 1) | signBit0;
	return output;
}

// Fragment/Pixel Shader
struct PSInput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	nointerpolation uint glyphKind : SV_InstanceID;
};

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET
{
	float4 texColor;
	if(pushConstants.texture.IsValid()) {
		texColor = pushConstants.texture.Sample2D<float4>(input.texCoord);
	} else {
		texColor = float4(1.0, 1.0, 1.0, 1.0);
	}

	float3 baseColor = float3(1.0, 1.0, 1.0);
	if(input.glyphKind == 1 || input.glyphKind == 2) {
		baseColor = texColor.rgb;
	}

	float4 color;
	color.xyz = baseColor * texColor.a; // Premultiply alpha for better blending
	color.w = texColor.a;

	return float4(color);
}
