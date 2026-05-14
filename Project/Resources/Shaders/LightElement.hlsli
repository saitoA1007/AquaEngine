#ifndef LIGHT_HLSLI
#define LIGHT_HLSLI

static const float PI = 3.14159265359f;

// 平行光源
struct DirectionalLight
{
    float32_t4 color; // ライトの色
    float32_t3 direction; // ライトの向き
    float32_t intensity; // 輝度
    int32_t active;
    uint32_t isDepthTexture; // 深度値を持ったテクスチャ
    float32_t2 padding;
    float32_t4x4 vpMatrix;
};

// ポイントライト
struct PointLight
{
    float32_t4 color; // ライトの色
    float32_t3 position; // ライトの位置
    float32_t intensity; // 輝度
    int32_t active; // 有効化
    float32_t radius; // ライトの届く最大距離
    float32_t decay; // 減衰率
};

// スポットライト
struct SpotLight
{
    float32_t4 color; // ライトの色
    float32_t3 position; // ライトの位置
    float32_t intensity; // 輝度
    float32_t3 direction; // ライトの方向
    float32_t distance; // ライトの最大距離
    float32_t decay; // 減衰率
    float32_t cosAngle; // 減衰率
    float32_t cosFalloffStart; // 
    int32_t active; // 有効化
};

// Lambert拡散反射
float3 CalcDiffuse(float3 normal, float3 lightDir, float3 lightColor, float3 albedo)
{
    float NdotL = dot(normal, lightDir);
    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    float3 diffuse = albedo * lightColor * cos;
    return diffuse;
}

// Blinn-Phong鏡面反射
float3 CalcSpecular(float3 normal, float3 lightDir, float3 viewDir,
    float3 lightColor, float3 specularColor, float shininess)
{
    float3 halfVector = normalize(lightDir + viewDir);
    float NDotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NDotH), shininess);
    float3 specular = lightColor * specularPow * specularColor;
    return specular;
}

float D_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * d * d);
}

float G_SchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotX / (NdotX * (1.0f - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float3 F_EnvApprox(float NdotV, float3 F0, float3 F90)
{
    return F0 + (F90 - F0) * pow(1.0f - NdotV, 5.0f);
}

float3 CalculateBRDF(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float roughness,
    float metallic
)
{
    // ハーフベクトル
    float3 H = normalize(V + L);

    // 金属
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    // BRDF項の計算
    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    float3 F = F_Schlick(HdotV, F0);
    
    // エネルギー保存
    float3 kS = F;
    float3 kD = (1.0f - kS);
     // 金属は拡散反射を持たない
    kD *= 1.0f - metallic;

    // Cook-Torrance鏡面反射項
    float3 numerator = D * G * F;
    float denominator = max(4.0f * NdotV * NdotL, 0.0001f);
    float3 specular = numerator / denominator;
    
    return (kD * albedo / PI + specular) * lightColor * NdotL;
}

float3 CalculateIBL(float3 albedo, float3 reflectColor, float3 N, float3 V, float metallic, float roughness)
{
    float NdotV = saturate(dot(N, V));
     // 金属
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 F90 = max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0);
    
    // 視線角度でのフレネル
    float3 F_env = F_EnvApprox(NdotV, F0, F90);
    float3 kD_env = (1.0f - F_env) * (1.0f - metallic);
    
    // 反射強度
    float reflectStrength = (1.0f - roughness) * (1.0f - roughness);
    
    // 反射色
    float3 tintedReflect = reflectColor * lerp(float3(1.0f, 1.0f, 1.0f), albedo, metallic);
    
    // 簡易アンビエント
    float3 ambient = albedo * 0.03f;
    
    return kD_env * ambient + F_env * reflectStrength * tintedReflect;
}
#endif