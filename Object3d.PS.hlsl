#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enavleLighting;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 dirction;
    float intesity;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);

Texture2D<float32_t4> gTexture : register(t0);

SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    if (gMaterial.enavleLighting != 0)
    {
        float cos = saturate(dot(normalize(input.normal), -gDirectionalLight.dirction));
        output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intesity;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }

    return output;
}