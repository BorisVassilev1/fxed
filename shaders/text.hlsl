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
	int2 viewportSize;
	int2 translation;
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
	// snap to pixel grid
	//output.position.xy = round(output.position.xy * pushConstants.viewportSize / 2.f) / (pushConstants.viewportSize / 2.f);

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

	float largeness = (pushConstants.textSize - 12.f) / 48.0f; // Assuming textSize ranges from 12 to 60
	largeness = clamp(largeness, 0.0f, 1.0f);

	//float Smoothing = lerp(1.0f, 0.5f, largeness);
	float Smoothing = 1.0f;

	float d = median(texColor.rgb) - 0.5f;

	float pxr = d/fwidth(d) ;
	float w = smoothstep(-Smoothing,Smoothing, pxr);

	//float4 insideColor = float4(243/255.f, 139/255.f, 168/255.f, 1.0);
	float4 insideColor = float4(1.0, 1.0, 1.0, 1.0);
	float4 outsideColor = float4(30/255.f, 30/255.f, 46/255.f, 0.0);
	//float4 outsideColor = float4(0,0,0, 1.0);
	float4 color = lerp(outsideColor, insideColor, w);

	// Calculate screen-space derivatives for proper antialiasing

	// gamma correction

	return float4(color);
}
