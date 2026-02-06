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
};

struct PushConstantsCursor
{
	int2 viewportSize;
	int2 translation;
	float2 cursorPosition;
	float textSize;
	float time;
};

VK_PUSH_CONST_ATTR
PushConstantsCursor pushConstants : register(b0);

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
	float2 scale = 2.f * pushConstants.textSize / float2(pushConstants.viewportSize);
    VSOutput output;
	output.position = float4(
		-1.f + scale * (input.position + pushConstants.translation + pushConstants.cursorPosition + float2(0.f, -0.3f))
		, 0.0, 1.0);

	output.texCoord = input.texCoord;
    return output;
}

struct PSInput
{
    float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET
{
	float4 color0 = float4(1.0, 1.0, 1.0, 1.0);
	float4 color1 = float4(0.0, 0.0, 0.0, 0.0);
	float alpha = step(0.5f, frac(pushConstants.time + 1.f));
	return float4(lerp(color0, color1, alpha));
}
