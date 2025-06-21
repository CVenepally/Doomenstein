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
    float		Intensity;
    //--------------------------------------16 bytes
	
	float4		Color;
	//--------------------------------------16 bytes	

    float		SpotAngle;
    float		ConstantAttenuation;
    float		LinearAttenuation;
    float		QuadraticAttenuation;
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
float4 CalculateSceneLightsColor(float4 worldPosition, float3 worldNormal)
{
    float4 sceneLightFinalColor = float4(0.f, 0.f, 0.f, 0.f);
	
	[unroll]
    for(int lightIndex = 0; lightIndex < NumLights; lightIndex++)
    {
        float3 lightVector = AllLights[lightIndex].Position - worldPosition.xyz;
        float distance = length(lightVector);
        float lightAttenuation = 1.f / (AllLights[lightIndex].ConstantAttenuation + AllLights[lightIndex].LinearAttenuation * distance + AllLights[lightIndex].QuadraticAttenuation * distance * distance);

        if (AllLights[lightIndex].LightType == POINT_LIGHT)
        {
            float pointLightDiffuse = AllLights[lightIndex].Intensity * saturate(dot(worldNormal, lightVector));
		
            sceneLightFinalColor += pointLightDiffuse * lightAttenuation * AllLights[lightIndex].Color;
        }
        else if (AllLights[lightIndex].LightType == SPOT_LIGHT)
        {
            float minCos = cos(AllLights[lightIndex].SpotAngle);
            float maxCos = (minCos + 1.0f) / 2.0f;
            float cosAngle = dot(AllLights[lightIndex].Direction.xyz, -lightVector);
			
            float spotAngleIntensity = smoothstep(minCos, maxCos, cosAngle);
			
            float spotDiffuse = AllLights[lightIndex].Intensity * saturate(dot(worldNormal, lightVector)) * spotAngleIntensity;
		
            sceneLightFinalColor += spotDiffuse * lightAttenuation * AllLights[lightIndex].Color;			
        }       
    }
	
    return float4(sceneLightFinalColor.xyz, 1);
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{	
    float3 normalizedWorldNormal = normalize(input.worldNormal.xyz);
	
	float  ambient			= AmbientIntensity;
    float  diffuse			= DirectionalLight.Intensity * saturate(dot(normalizedWorldNormal, -DirectionalLight.Direction));
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
	
    float4 finalLightColor = lightColor + sceneLightsFinalColor;
    float4 color = finalLightColor * textureColor * vertexColor * modelColor;
	clip(color.a - 0.01f);
	
    return color;
	
}
