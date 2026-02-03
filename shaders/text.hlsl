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

struct PushConstants
{
	float4 translate_scale; // xy = translate, zw = scale
	TextureHandle texture;
};

VK_PUSH_CONST_ATTR
PushConstants pushConstants : register(b0);

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
	float2 translate = pushConstants.translate_scale.xy;
	float2 scale = pushConstants.translate_scale.zw;

    VSOutput output;
	output.position = float4(scale * input.position + translate, 0.0, 1.0);
	output.texCoord = input.texCoord;
    return output;
}

// Fragment/Pixel Shader
struct PSInput
{
    float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

float median(float3 msd)
{
    return max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b));
}


[shader("pixel")]
float4 PSMain(PSInput input) : SV_TARGET
{
	float4 texColor;
	if(pushConstants.texture.IsValid()) {
		texColor = pushConstants.texture.Sample2D<float4>(input.texCoord);
	} else {
		texColor = float4(1.0, 1.0, 1.0, 1.0);
	}

	float PixelRange = 16; // Distance field range in pixels
	float Smoothing = 0.3;  // Edge smoothing factor

	float d = median(texColor.rgb) - 0.5f;
	float w = clamp(d/fwidth(d) + 0.5, 0.0, 1.0);

	float4 insideColor = float4(1.0, 1.0, 1.0, 1.0);
	float4 outsideColor = float4(1.0, 0.0, 0.0, 0.0);
	float4 color = lerp(outsideColor, insideColor, w);

	// Calculate screen-space derivatives for proper antialiasing

	//return float4(input.texCoord, 0.0, 1.0);
	return float4(color);
	//return float4(1.0, 1.0, 1.0, 1.0);
}
