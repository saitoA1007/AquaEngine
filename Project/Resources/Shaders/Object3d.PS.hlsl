#include"Object3d.hlsli"
#include"LightElement.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
    float32_t3 specularColor;
    float shininess;
    uint32_t textureHandle;
    float metallic;
    int32_t isActiveShadow;
};
ConstantBuffer<Material> gMaterial : register(b0);

Texture2D<float32_t4> gTexture[] : register(t0,space0);
TextureCube<float32_t4> gCubeTexture[] : register(t1,space1);
Texture2D<float> gShadowMap[] : register(t2,space2);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct Camera
{
    float32_t3 worldPosition;
    float32_t4x4 vpMatrix;
};
ConstantBuffer<Camera> gCamera : register(b1);

cbuffer LightGroup : register(b2)
{
    DirectionalLight gDirectionalLight;
    PointLight gPointLight;
    SpotLight gSpotLight;
    uint32_t environmentTexture;
    int32_t isActiveEnvironment;
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// ライトの処理
float32_t3 CalculateShading(float32_t3 lightDirection, float32_t3 lightColor, float32_t3 normal, float32_t3 viewDirection, float32_t3 materialColor, Material matData);
float32_t3 CalculateDirectionalLight(DirectionalLight light,float32_t3 normal,float32_t3 viewDirection,float32_t3 materialColor,Material matData);
float32_t3 CalculatePointLight(PointLight light,float32_t3 worldPosition,float32_t3 normal,float32_t3 viewDirection,float32_t3 materialColor,Material matData);
float32_t3 CalculateSpotLight(SpotLight light, float32_t3 worldPosition, float32_t3 normal, float32_t3 viewDirection, float32_t3 materialColor, Material matData);
float32_t3 CalculateEnvironmentMap(float32_t3 worldPosition, float32_t3 normal, float32_t3 cameraPosition);
// 影を計算する
float32_t CalculateShadow(float32_t4 shadowCoord);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture[gMaterial.textureHandle].Sample(gSampler, transformedUV.xy);
    
    if (textureColor.a == 0.0)
    {
        discard;
    }
    
    if (gMaterial.enableLighting)
    { // Lightingする場合
        
        // 最終的な色
        float32_t3 finalColor = float32_t3(0.0f, 0.0f, 0.0f);
        
        // ライト計算のための共通データを準備
        float32_t3 normal = normalize(input.normal);
        float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
        float32_t3 baseColor = gMaterial.color.rgb * textureColor.rgb;
        
        if (gDirectionalLight.active)
        {
            // デフォルトは影なし
            float finalShadow = 1.0f;
            
            if (gMaterial.isActiveShadow)
            {
                float4 world = float4(input.worldPosition, 1.0f);
                float4 shadowCoord = mul(world, gDirectionalLight.vpMatrix);
                // 影の計算を実行
                float32_t shadowFactor = CalculateShadow(shadowCoord);
                
                float shadowAtten = 1.0f - 0.8f; // 影部分の明るさ
                finalShadow = shadowFactor + shadowAtten * (1.0f - shadowFactor);
            }
            
            finalColor += CalculateDirectionalLight(gDirectionalLight, normal, toEye, baseColor, gMaterial) * finalShadow;
        }
        
        if (gPointLight.active)
        {
            finalColor += CalculatePointLight(gPointLight, input.worldPosition, normal, toEye, baseColor, gMaterial);  
        }         
        
        if (gSpotLight.active)
        {
            finalColor += CalculateSpotLight(gSpotLight, input.worldPosition, normal, toEye, baseColor, gMaterial);
        }
        
        // 環境マップを適応
        if (isActiveEnvironment)
        {
            float32_t3 reflectedVector = CalculateEnvironmentMap(input.worldPosition, normal, gCamera.worldPosition);
            float32_t4 environmentColor = gCubeTexture[environmentTexture].Sample(gSampler, reflectedVector);
            finalColor += environmentColor.rgb * gMaterial.metallic;
        }
        
        // 最終的な色を設定
        output.color.rgb = finalColor;
       
        // アルファ値を適応
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    { // Lighttingしない場合。
        output.color = gMaterial.color * textureColor;
    }
    
    if (output.color.a == 0.0)
    {
        discard;
    }
    
    return output;
}

// Blinn-Phong + Half-Lambertの計算
float32_t3 CalculateShading(
    float32_t3 lightDirection, // ライトへの方向ベクトル
    float32_t3 lightColor,     // ライトの色 * 強度
    float32_t3 normal,         // 法線
    float32_t3 viewDirection,  // カメラへの方向
    float32_t3 materialColor,  // マテリアル色 * テクスチャ色
    Material matData)          // マテリアル構造体
{
    // Diffuse(Half-Lambert)
    float NdotL = dot(normal, lightDirection);
    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    float32_t3 diffuse = materialColor * lightColor * cos;

    // Specular(Blinn-Phong)
    float32_t3 halfVector = normalize(lightDirection + viewDirection);
    float NDotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NDotH), matData.shininess);
    float32_t3 specular = lightColor * specularPow * matData.specularColor;

    return diffuse + specular;
}

// DirectionalLightの計算
float32_t3 CalculateDirectionalLight(
    DirectionalLight light,
    float32_t3 normal,
    float32_t3 viewDirection,
    float32_t3 materialColor,
    Material matData)
{
    // 平行光源なので、ライトへの方向は -direction
    float32_t3 lightDir = normalize(-light.direction);
    float32_t3 lightColorIntensity = light.color.rgb * light.intensity;

    return CalculateShading(lightDir, lightColorIntensity, normal, viewDirection, materialColor, matData);
}

// PointLightの計算
float32_t3 CalculatePointLight(
    PointLight light,
    float32_t3 worldPosition,
    float32_t3 normal,
    float32_t3 viewDirection,
    float32_t3 materialColor,
    Material matData)
{
    // ポイントライトへの方向ベクトルと距離
    float32_t3 directionToLight = light.position - worldPosition;
    float32_t distance = length(directionToLight);
    float32_t3 lightDir = normalize(directionToLight);
    // 距離減衰
    float32_t factor = pow(saturate(-distance / light.radius + 1.0), light.decay);
    float32_t3 lightColorIntensity = light.color.rgb * light.intensity * factor;

    return CalculateShading(lightDir, lightColorIntensity, normal, viewDirection, materialColor, matData);
}

// SpotLightの計算
float32_t3 CalculateSpotLight(
    SpotLight light,
    float32_t3 worldPosition,
    float32_t3 normal,
    float32_t3 viewDirection,
    float32_t3 materialColor,
    Material matData)
{
    // スポットライト光源位置への方向
    float32_t3 directionToLight = light.position - worldPosition;
    float32_t distance = length(directionToLight);
    float32_t3 lightDirOnSurface = normalize(directionToLight);
    // 角度減衰
    float32_t cosAngle = dot(-lightDirOnSurface, normalize(light.direction));
    float32_t falloffFactor = saturate((cosAngle - light.cosAngle) / (light.cosFalloffStart - light.cosAngle));
    // 距離減衰
    float attenuationFactor = pow(1.0f / distance, light.decay) * saturate(1.0f - distance / light.distance);
    // 最終的な強さ
    float32_t3 lightColorIntensity = light.color.rgb * light.intensity * attenuationFactor * falloffFactor;

    float32_t3 shadingDir = normalize(-light.direction);
    
    return CalculateShading(shadingDir, lightColorIntensity, normal, viewDirection, materialColor, matData);
}

// 環境マップの計算
float32_t3 CalculateEnvironmentMap(float32_t3 worldPosition, float32_t3 normal, float32_t3 cameraPosition)
{
    float32_t3 cameraToPosition = normalize(worldPosition - cameraPosition);
    float32_t3 reflectedVector = reflect(cameraToPosition, normal);
    return reflectedVector;
}

float32_t CalculateShadow(float32_t4 shadowCoord)
{
    // 透視投影除算
    float32_t3 projectCoord = shadowCoord.xyz / shadowCoord.w;

    // uv座標に変換
    projectCoord.x = projectCoord.x * 0.5f + 0.5f;
    projectCoord.y = -projectCoord.y * 0.5f + 0.5f;

    // テクスチャ範囲外かチェック
    if (projectCoord.x < 0.0f || projectCoord.x > 1.0f ||
        projectCoord.y < 0.0f || projectCoord.y > 1.0f ||
        projectCoord.z < 0.0f || projectCoord.z > 1.0f)
    {
        return 1.0f;
    }

    float32_t bias = 0.001f;
    float currentDepth = projectCoord.z - bias;
    
    // ShadowMapサンプリング
    return gShadowMap[gDirectionalLight.isDepthTexture].SampleCmpLevelZero(gShadowSampler, projectCoord.xy, currentDepth);
}