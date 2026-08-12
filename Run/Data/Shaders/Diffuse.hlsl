#define MAX_LIGHTS 64
#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 modelTangent : TANGENT;
	float3 modelBitangent : BITANGENT;
	float3 modelNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 clipPosition   : SV_Position;
	float4 color		  : COLOR;
	float2 uv			  : TEXCOORD;
	float4 worldTangent	  : TANGENT;
	float4 worldBitangent : BITANGENT;
	float4 worldNormal    : NORMAL;	
    float4 worldPosition  : POSITION;
	
};

//------------------------------------------------------------------------------------------------
struct Light
{
    int			LightType;
    float3		Position;
	//--------------------------------------16 bytes
	
    float3		Direction;
    float		Padding0;
    //--------------------------------------16 bytes

	float4		Color;			// alpha is intensity
	//--------------------------------------16 bytes

    float		InnerRadius;
    float		OuterRadius;
    float		InnerDot;
    float		OuterDot;
	//-------------------------------------16 bytes
};

//------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b4)
{
    Light   DirectionalLight;			  // 64 bytes
    Light	AllLights[MAX_LIGHTS];		  // 64 bytes
	
    float3	CameraPosition;
	int  	NumLights; 	
	//----------------------16
	
	float  AmbientIntensity;
    float3 DummyPadding;
	//-----------------------16	
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 WorldToCameraTransform;	// View transform
	float4x4 CameraToRenderTransform;	// Non-standard transform from game to DirectX conventions
	float4x4 RenderToClipTransform;		// Projection transform
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelToWorldTransform;		// Model transform
	float4	 ModelColor;
};

//------------------------------------------------------------------------------------------------
cbuffer ShadowConstants : register(b4)
{
    float4x4	LightWorldToClipTransform;
    float2		TexelSize;
	float		DepthBias;
	float		Padding;
};

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture	: register(t0);
Texture2D shadowMapTexture	: register(t1);

//------------------------------------------------------------------------------------------------
SamplerState samplerState						: register(s0);
SamplerComparisonState samplerComparisonState	: register(s1);

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 modelPosition	= float4(input.modelPosition, 1);
	float4 worldPosition	= mul(ModelToWorldTransform,   modelPosition);
	float4 cameraPosition	= mul(WorldToCameraTransform,  worldPosition);
	float4 renderPosition	= mul(CameraToRenderTransform, cameraPosition);
	float4 clipPosition		= mul(RenderToClipTransform,   renderPosition);

	float4 worldTangent		= mul(ModelToWorldTransform, float4(input.modelTangent, 0.0f));
	float4 worldBitangent	= mul(ModelToWorldTransform, float4(input.modelBitangent, 0.0f));
	float4 worldNormal		= mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));

	v2p_t v2p;
	v2p.clipPosition   = clipPosition;
	v2p.color		   = input.color;
	v2p.uv			   = input.uv;
	v2p.worldTangent   = worldTangent;
	v2p.worldBitangent = worldBitangent;
	v2p.worldNormal    = worldNormal;
    v2p.worldPosition  = worldPosition;
	
	return v2p;
}

//------------------------------------------------------------------------------------------------
float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
    float fraction = saturate((inValue - inStart) / (inEnd - inStart));
    return outStart + fraction * (outEnd - outStart);
}

//------------------------------------------------------------------------------------------------
float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
    float fraction = (inValue - inStart) / (inEnd - inStart);
    return outStart + fraction * (outEnd - outStart);
}

//------------------------------------------------------------------------------------------------
float4 CalculateSceneLightsColor(float4 worldPosition, float3 worldNormal)
{
    float4 sceneLightFinalColor = float4(0.f, 0.f, 0.f, 0.f);
	
	[unroll]
    for(int lightIndex = 0; lightIndex < NumLights; lightIndex++)
    {
        float3 lightVector    = AllLights[lightIndex].Position - worldPosition.xyz;
        float  distance       = length(lightVector);
        float3 lightDirection = normalize(lightVector);

        float  lightIntensity = AllLights[lightIndex].Color.a;

        // full brightness within InnerRadius, falling off to zero at OuterRadius
        float  falloff        = RangeMapClamped(distance, AllLights[lightIndex].OuterRadius, AllLights[lightIndex].InnerRadius, 0.f, 1.f);
        float  normalDot      = RangeMapClamped(dot(worldNormal, lightDirection), -AmbientIntensity, 1.f, 0.f, 1.f);

        if (AllLights[lightIndex].LightType == POINT_LIGHT)
        {
            float pointLightDiffuse = lightIntensity * normalDot * falloff;
		
            sceneLightFinalColor += pointLightDiffuse * AllLights[lightIndex].Color;
        }
        else if (AllLights[lightIndex].LightType == SPOT_LIGHT)
        {
            float spotDirNormalDot = dot(AllLights[lightIndex].Direction.xyz, -lightDirection);
			
            float spotAngleIntensity = saturate(RangeMap(spotDirNormalDot, AllLights[lightIndex].OuterDot, AllLights[lightIndex].InnerDot, 0.f, 1.f));
			
            float spotDiffuse = lightIntensity * normalDot * falloff * spotAngleIntensity;
		
            sceneLightFinalColor += spotDiffuse * AllLights[lightIndex].Color;			
        }       
    }
	
    return float4(sceneLightFinalColor.xyz, 1);
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{	
    float3 normalizedWorldNormal = normalize(input.worldNormal.xyz);
	
	float  ambient			= AmbientIntensity;
    float  diffuse			= DirectionalLight.Color.a * saturate(dot(normalizedWorldNormal, -DirectionalLight.Direction));
    float4 lightColor		= float4((ambient + diffuse).xxx, 1);
	float4 textureColor		= diffuseTexture.Sample(samplerState, input.uv);
	float4 vertexColor		= input.color;
	float4 modelColor		= ModelColor;

    float4 sceneLightsFinalColor = CalculateSceneLightsColor(input.worldPosition, normalizedWorldNormal);
	
 //   float4 shadowPosition = mul(LightWorldToClipTransform, float4(input.worldPosition.xyz, 1.0f));
 //   shadowPosition.xyz /= shadowPosition.w;
	
 //   float2 shadowUV = float2(shadowPosition.x, -shadowPosition.y) * 0.5f + 0.5f;
	
 //   float shadow = 0.f;
	
	//[unroll]
 //   for (int x = -1; x <= 1; ++x)
 //   {
	//	[unroll]
 //       for (int y = -1; y <= 1; ++y)
 //       {
 //           float2 offset = float2(x, y) * TexelSize;
 //           shadow += shadowMapTexture.SampleCmp(samplerComparisonState, shadowUV + offset, shadowPosition.z - DepthBias);
 //       }
 //   }
	
 //   shadow /= 9.0f;
	
    // saturate the accumulated lighting so overlapping colored lights cannot
    // sum past white and bleed into hues that were never authored (e.g. magenta)
    float4 finalLightColor = saturate(lightColor + sceneLightsFinalColor);
    float4 color = finalLightColor * textureColor * vertexColor * modelColor;
	clip(color.a - 0.01f);
	
    return color;
	
}
