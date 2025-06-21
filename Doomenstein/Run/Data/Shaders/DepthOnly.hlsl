struct VertexShaderInput
{
	float3 modelPosition : POSITION;
};

//------------------------------------------------------------------------------------------------
struct PixelShaderInput
{
	float4 clipPosition   : SV_Position;	
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 WorldToCameraTransform;
	float4x4 CameraToRenderTransform;
	float4x4 RenderToClipTransform;	
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelToWorldTransform;
	float4	 ModelColor;
};

//------------------------------------------------------------------------------------------------
PixelShaderInput VertexMain(VertexShaderInput input)
{
	float4 modelPosition	= float4(input.modelPosition, 1);
	float4 worldPosition	= mul(ModelToWorldTransform,   modelPosition);
	float4 cameraPosition	= mul(WorldToCameraTransform,  worldPosition);
	float4 renderPosition	= mul(CameraToRenderTransform, cameraPosition);
	float4 clipPosition		= mul(RenderToClipTransform,   renderPosition);

	PixelShaderInput vertexToPixel;
    vertexToPixel.clipPosition = clipPosition;
		
    return vertexToPixel;
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(PixelShaderInput input) : SV_Target0
{
    return float4(1.f, 1.f, 1.f, 1.f);
}
