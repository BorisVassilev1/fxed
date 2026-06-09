#include <resource_heap.hlsl>

// Vertex Shader
struct VSInput
{
	float4 color : COLOR;
	float2 translation : TRANSLATION;
	int2 charIndexDrawMode : CHAR_INDEX_DRAW_MODE;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
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
	ArrayBufferHandle glyphDataBuffer;
};

struct Rectangle
{
	int x, y, w, h;
};

struct GlyphBox
{
	struct {
		float l, b, r, t;
	} bounds;
	Rectangle rect;
	int index;
	float advance;
	bool isBitmap;

	int fallbackFontIndex;
};

VK_PUSH_CONST_ATTR
PushConstants pushConstants : register(b0);

[shader("vertex")]
VSOutput VSMain(VSInput input, uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	float2 scale = 2.f * pushConstants.textSize / float2(pushConstants.viewportSize);

	GlyphBox glyphBox = pushConstants.glyphDataBuffer.Load<GlyphBox>(input.charIndexDrawMode.x);

	float2 position = float2(0.0, 0.0);
	float2 positions[4] = {
		float2(glyphBox.bounds.l, -glyphBox.bounds.t),
		float2(glyphBox.bounds.l, -glyphBox.bounds.b),
		float2(glyphBox.bounds.r, -glyphBox.bounds.b),
		float2(glyphBox.bounds.r, -glyphBox.bounds.t)
	};
	position = positions[vertexID % 4];

	float2 texCoords[4] = {
		float2(glyphBox.rect.x, glyphBox.rect.y + glyphBox.rect.h),
		float2(glyphBox.rect.x, glyphBox.rect.y),
		float2(glyphBox.rect.x + glyphBox.rect.w, glyphBox.rect.y),
		float2(glyphBox.rect.x + glyphBox.rect.w, glyphBox.rect.y + glyphBox.rect.h)
	};

	VSOutput output;
	output.position = float4(-1.f + scale * (position + input.translation + pushConstants.translation), 1.0, 1.0);
	output.color = input.color;
	output.texCoord = abs(texCoords[vertexID % 4] / float2(512.0, 512.0));

	output.glyphKind = input.charIndexDrawMode.y;
	return output;
}

// Fragment/Pixel Shader
struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
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
	} else {
		baseColor = input.color.rgb;
	}

	float4 color;
	color.xyz = baseColor * texColor.a; // Premultiply alpha for better blending
	color.w = texColor.a;

	return float4(color);
}
