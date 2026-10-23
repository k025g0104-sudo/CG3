#include "Particle.hlsli"

struct Material
{
    float4 color;
    int lightingType;
    float3 padding;
    float4x4 uvTransform;
};

struct DirectionalLight
{
    float4 color;
    float3 dirction;
    float intesity;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    // * float32_t4(ligthFactor, 1.0f);
    output.color = gMaterial.color * textureColor * input.color;
   // float32_t3 ligthFactor = float32_t3(1.0f, 1.0f, 1.0f);

    if (output.color.a == 0)
    {
        discard;
    }
    return output;
    
    //if (gMaterial.lightingType == 1)
    //{
    //    float NdotL = dot(normalize(input.normal), -gDirectionalLight.dirction);
    //    float cos = saturate(NdotL);
    //    ligthFactor = gDirectionalLight.color.rgb * cos * gDirectionalLight.intesity;
    //}
    //else if (gMaterial.lightingType == 2)
    //{
    //    float NdotL = dot(normalize(input.normal), -gDirectionalLight.dirction);
    //    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    //    ligthFactor = gDirectionalLight.color.rgb * cos * gDirectionalLight.intesity;
    //}

    //if (textureColor.a <= 0.5f)
    //{
    //    discard;
    //}
        
    //if (textureColor.a == 0.0f)
    //{
    //    discard;
    //}
    
    //if (output.color.a == 0.0f)
    //{
    //    discard;
    //}
    
    
    //output.color.rgb = gMaterial.color.rgb * textureColor.rgb * ligthFactor;
    //output.color.a = gMaterial.color.a * textureColor.a;
    
    //return output;
}