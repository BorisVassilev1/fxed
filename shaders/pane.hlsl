#include <resource_heap.hlsl>
// Vertex Shader
struct VSInput {
	float2 position : POSITION;
	float2 texCoord : TEXCOORD0_;
};

struct VSOutput {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

struct PushConstants {
	float3 color0;
	int borderSize;
	float3 color1;
	float time;
	int2 viewportSize;
};

VK_PUSH_CONST_ATTR
PushConstants pushConstants : register(b0);

[shader("vertex")]
VSOutput VSMain(VSInput input) {
	VSOutput output;
	output.position = float4(input.position, 0.0, 1.0);
	output.texCoord = input.texCoord;
	return output;
}

struct PSInput {
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET {
	// border size is in pixels, so we need to convert it to UV space
	float2 borderSizeUV = pushConstants.borderSize / float2(pushConstants.viewportSize);
	// calculate distance from the edge of the pane
	float2 distanceFromEdge = min(input.texCoord, 1.0 - input.texCoord);
	float alpha = step(borderSizeUV.x, distanceFromEdge.x) * step(borderSizeUV.y, distanceFromEdge.y);
	return float4(lerp(pushConstants.color1, pushConstants.color0, alpha), 1.0);
}
