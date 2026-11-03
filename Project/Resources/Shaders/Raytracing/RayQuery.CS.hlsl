#include "Common.hlsli"
#include "../LightElement.hlsli"
#include "RtMaterials.hlsli"

// RayQueryによるレイトレーシング
// MainRayQueryCS  : DXRのRayGen/ClosestHitの再帰を、レイのスタックを使ったループに置き換えたもの
//                   見た目はMainRayGen + MainObjectCHS/MainIceObjectCHS と同じになる
// MainPathTraceCS : 1画素1パスのパストレーシング。直接光(NEE) + BSDFサンプリング + ロシアンルーレット

// フレームごとに変わる値
cbuffer RayQueryConstants : register(b2)
{
    uint gFrameIndex; // 乱数のシードに使うフレーム番号
    uint gUseVisibilityBuffer; // 1なら最初のヒットをビジビリティバッファから求める
};

// ラスタライズしたビジビリティバッファ。x:TLASのインスタンス番号+1(0なら何も無い), y:三角形の番号
Texture2D<uint2> gVisibilityBuffer : register(t2);
// TLASのインスタンス情報(D3D12_RAYTRACING_INSTANCE_DESCの配列)
ByteAddressBuffer gInstanceDescs : register(t3);

// デノイズ用のガイドバッファ。パストレーシングの最初のヒットの情報を書き込む
RWTexture2D<float4> gGuideOutput : register(u2); // xyz:ワールド法線, w:カメラからの距離(0なら背景)
RWTexture2D<float4> gAlbedoOutput : register(u3); // rgb:アルベド。デノイズ前に光をこの色で割る

// 再帰の上限。この回数目のヒットでは背景を返す(checkRecursiveLimitと同じ)
static const int RQ_MAX_RECURSIVE = 4;
// 1画素で処理するレイの最大数
static const int RQ_MAX_RAYS = 16;
// レイのスタックの大きさ
static const int RQ_STACK_SIZE = 8;

// パストレーシングの最大バウンス数
static const int PT_MAX_BOUNCE = 8;
// このバウンス以降でロシアンルーレットを行う
static const int PT_RR_START_BOUNCE = 2;
// 2バウンス目以降の寄与の上限。ホタル状のノイズを抑える
static const float PT_MAX_INDIRECT_LUMINANCE = 8.0f;

// ヒットグループのインデックス。SetHitGroupで設定した値
static const uint HIT_GROUP_DEFAULT = 0;
static const uint HIT_GROUP_ICE = 1;

// 追跡待ちのレイ
struct RayTask
{
    float3 origin;
    float3 direction;
    float3 weight; // このレイが持ち帰る色に掛ける重み
    int recursive; // 親のヒットまでの再帰回数
    uint flags; // TraceRayInlineに渡すレイフラグ
};

// RayQueryで得たヒット情報
struct HitInfo
{
    uint instanceID;
    uint primitiveIndex;
    uint hitGroupIndex;
    float2 barys;
    float t;
    float4x3 objectToWorld;
    float4x3 worldToObject;
};

// シェーディング結果。自身の色と、追加で飛ばすレイ(最大2本)
struct ShadeResult
{
    float3 color;
    int childCount;
    RayTask children[2];
};

// 通常オブジェクトの表面情報
struct ObjectSurface
{
    MaterialData material;
    uint type; // BufferRef.type。1なら透明
    float3 position;
    float3 normal; // 視線側に向けた法線
    float3 albedo;
    float depth;
};

// 氷の表面情報
struct IceSurface
{
    IceMaterialData material;
    float3 position;
    float3 normal; // 向きを揃えていない法線
    float3 iceColor;
    float3 bubbleColor;
    float depth;
};

bool TraceClosest(float3 origin, float3 direction, uint flags, out HitInfo hit)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001f;
    ray.TMax = 100000.0f;

    RayQuery<RAY_FLAG_FORCE_OPAQUE> q;
    q.TraceRayInline(gRtScene, flags, 0xFF, ray);
    while (q.Proceed()) {}

    hit = (HitInfo) 0;
    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        return false;
    }
    hit.instanceID = q.CommittedInstanceID();
    hit.primitiveIndex = q.CommittedPrimitiveIndex();
    hit.hitGroupIndex = q.CommittedInstanceContributionToHitGroupIndex();
    hit.barys = q.CommittedTriangleBarycentrics();
    hit.t = q.CommittedRayT();
    hit.objectToWorld = q.CommittedObjectToWorld4x3();
    hit.worldToObject = q.CommittedWorldToObject4x3();
    return true;
}

float3 SampleBackground(float3 direction)
{
    return gBackgroundTexture.SampleLevel(gSampler, direction, 0.0).rgb;
}

float CalcDepth(float3 worldPosition)
{
    float4 clipPos = mul(float4(worldPosition, 1.0f), gCamera.vpMatrix);
    return clipPos.z / clipPos.w;
}

// ヒットした三角形の頂点を補間する
// applyOffset : インデックス・頂点のオフセットを使うか
VertexData LoadHitVertex(BufferRef ref, HitInfo hit, bool applyOffset)
{
    uint start = hit.primitiveIndex * 3;
    uint indexOffset = applyOffset ? ref.indexOffset : 0;
    uint vertexOffset = applyOffset ? ref.vertexOffset : 0;

    float3 positions[3];
    float2 texcoords[3];
    float3 normals[3];
    float4 tangents[3];

    for (int i = 0; i < 3; ++i)
    {
        uint localIndex = gBufferData[ref.indexHandle].Load<uint>((start + i) * 4 + indexOffset * 4);
        uint index = localIndex + vertexOffset;
        VertexData v = gBufferData[ref.vertexHandle].Load<VertexData>(index * VERTEX_STRIDE);

        positions[i] = v.position.xyz;
        normals[i] = v.normal;
        texcoords[i] = v.texcoord;
        tangents[i] = v.tangent;
    }

    VertexData v = (VertexData) 0;
    v.position.xyz = CalcHitAttribute3(positions, hit.barys);
    v.position.w = 1.0f;
    v.texcoord = CalcHitAttribute2(texcoords, hit.barys);
    v.normal = normalize(CalcHitAttribute3(normals, hit.barys));
    v.tangent = CalcHitAttribute4(tangents, hit.barys);
    return v;
}

// 通常オブジェクトの表面情報を取得する(MainObjectCHSの前半と同じ)
// viewDir : 法線を向ける視線方向
ObjectSurface LoadObjectSurface(HitInfo hit, float3 viewDir)
{
    ObjectSurface s;
    BufferRef ref = gBufferRefs[hit.instanceID];
    s.material = gBufferData[ref.MaterialIndex].Load<MaterialData>(0);
    s.type = ref.type;

    // MainObjectCHSのGetHitVertexはオフセットを使っていないため、それに合わせる
    VertexData vtx = LoadHitVertex(ref, hit, false);
    float4 transformedUV = mul(float4(vtx.texcoord, 0.0f, 1.0f), s.material.uvTransform);

    float3 localNormal = vtx.normal;
    if (s.material.normalTextureHandle != 0)
    {
        float4 normalMapColor = gTexture[s.material.normalTextureHandle].SampleLevel(gSampler, transformedUV.xy, 0);
        vtx.tangent.xyz = normalize(vtx.tangent.xyz);
        localNormal = GetNormalFromMap(normalMapColor, vtx.normal, vtx.tangent);
    }
    s.position = mul(vtx.position, hit.objectToWorld);
    float3x3 normalMatrix = transpose((float3x3) hit.worldToObject);
    s.normal = normalize(mul(localNormal, normalMatrix));
    if (dot(s.normal, viewDir) < 0.0f) { s.normal = -s.normal; }
    s.depth = CalcDepth(s.position);

    float4 textureColor = gTexture[s.material.textureHandle].SampleLevel(gSampler, transformedUV.xy, 0);
    s.albedo = s.material.color.rgb * textureColor.rgb;
    return s;
}

// 氷の表面情報を取得する(MainIceObjectCHSの前半と同じ)
// viewDir : 視差マッピングや気泡に使う視線方向
IceSurface LoadIceSurface(HitInfo hit, float3 viewDir)
{
    IceSurface s;
    BufferRef ref = gBufferRefs[hit.instanceID];
    IceMaterialData material = gBufferData[ref.MaterialIndex].Load<IceMaterialData>(0);
    s.material = material;

    VertexData vtx = LoadHitVertex(ref, hit, true);
    float4 transformedUV = mul(float4(vtx.texcoord, 0.0f, 1.0f), material.uvTransform);
    vtx.tangent.xyz = normalize(vtx.tangent.xyz);

    s.position = mul(vtx.position, hit.objectToWorld);
    s.depth = CalcDepth(s.position);

    float2 parallaxUV = transformedUV.xy;
    float3x3 worldToObjectRot = (float3x3) hit.worldToObject;
    float3 localViewDir = normalize(mul(viewDir, worldToObjectRot));
    if (material.heightTextureHandle != 0)
    {
        float3 N = normalize(vtx.normal);
        float3 T = vtx.tangent.xyz;
        float3 B = normalize(cross(N, T) * vtx.tangent.w);
        float3x3 tbn = float3x3(T, B, N);
        float3 tangentViewDir = normalize(mul(tbn, localViewDir));

        parallaxUV = ParallaxOcclusionMapping(
            gTexture[material.heightTextureHandle], gSampler,
            transformedUV.xy, tangentViewDir, material.heightScale);
    }

    s.bubbleColor = float3(0.0f, 0.0f, 0.0f);
    if (material.bubbleMaxDepth > 0.0001f)
    {
        float3 incident = -localViewDir;
        float3 refractedDir = refract(incident, vtx.normal, 1.0f / material.ior);
        if (dot(refractedDir, refractedDir) < 0.0001f)
        {
            refractedDir = incident;
        }

        float3 bubbleHitPos, bubbleHitNormalLocal;
        float hitDistance;
        bool hitBubble = ParallaxBubbleMapping(
            vtx.position.xyz, -refractedDir,
            material.bubbleScale, material.bubbleMaxDepth,
            material.bubbleJitter, material.bubbleDensity,
            bubbleHitPos, bubbleHitNormalLocal, hitDistance);

        if (hitBubble)
        {
            float3x3 normalMatrix = transpose((float3x3) hit.worldToObject);
            float3 bubbleNormalWorld = normalize(mul(normalMatrix, bubbleHitNormalLocal));

            float NdotV = saturate(dot(bubbleNormalWorld, viewDir));
            float3 lightDir = normalize(-gDirectionalLight.direction);
            float3 halfVec = normalize(lightDir + viewDir);

            float bubbleDiffuse = saturate(dot(bubbleNormalWorld, lightDir)) * 0.4f;
            float bubbleSpec = pow(saturate(dot(bubbleNormalWorld, halfVec)), 48.0f);
            float bubbleRim = pow(1.0f - NdotV, 3.0f) * 0.3f;

            s.bubbleColor = float3(1.0f, 1.0f, 1.0f) * (bubbleDiffuse + bubbleSpec + bubbleRim) * material.bubbleHighlight;
            s.bubbleColor *= BeerLambert(float3(0.15f, 0.05f, 0.02f), hitDistance);
        }
    }

    float3 localNormal = vtx.normal;
    if (material.normalTextureHandle != 0)
    {
        float4 normalMapColor = gTexture[material.normalTextureHandle].SampleLevel(gSampler, parallaxUV, 0);
        localNormal = GetNormalFromMap(normalMapColor, vtx.normal, vtx.tangent);
    }

    if (material.dissolveThreshold > 0.0f)
    {
        float mask = FBMNoise(vtx.position.xyz * 2.0f, 1);
        if (mask <= material.dissolveThreshold)
        {
            localNormal = ChiseledIceNormal(localNormal, vtx.position.xyz,
                                    material.chipScale, material.chipStrength,
                                    material.edgeWidth, material.edgeStrength,
                                    material.microScale, material.microStrength);
        }
    }

    s.normal = normalize(mul(localNormal, (float3x3) hit.objectToWorld));

    float4 textureColor = gTexture[material.textureHandle].SampleLevel(gSampler, parallaxUV, 0);
    s.iceColor = material.color.rgb * textureColor.rgb;
    return s;
}

// 氷の表面で光源から直接届く成分(鏡面ハイライト・リムライト・気泡)
// normal : 視線側に向けた法線
float3 IceSurfaceLighting(IceSurface s, float3 normal, float3 viewDir, float shadowFactor)
{
    float3 color = float3(0.0f, 0.0f, 0.0f);
    float3 lightDir = normalize(-gDirectionalLight.direction);

    // 平行光源による鏡面ハイライト
    if (gDirectionalLight.active)
    {
        float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        float3 iceSpecular = CalcSpecular(normal, lightDir, viewDir,
            lightColor, s.material.specularColor.rgb, s.material.shininess);
        color += iceSpecular * shadowFactor;
    }

    // リムライト
    float rimFactor = pow(1.0f - saturate(dot(normal, viewDir)), s.material.rimPower);
    float3 rimLight = s.material.rimColor.rgb * rimFactor * s.material.rimIntensity * gDirectionalLight.intensity;
    color += rimLight * shadowFactor;

    // 気泡
    float3 F0Ice = float3(0.02f, 0.02f, 0.02f);
    float surfaceFresnel = F_Schlick(saturate(dot(normal, viewDir)), F0Ice).x;
    color += s.bubbleColor * (1.0f - surfaceFresnel) * shadowFactor;
    return color;
}

// 氷のフレネル反射率。IceBSDFと同じく見た目のためにr0の下限を上げている
float IceFresnel(float cosTheta, float ior)
{
    float r0 = (1.0f - ior) / (1.0f + ior);
    r0 = max(r0 * r0, 0.3f);
    return saturate(r0 + (1.0f - r0) * pow(1.0f - saturate(cosTheta), 5.0f));
}

// 氷の内部を進んだ距離による吸収
float3 IceTransmittance(IceSurface s, float distance)
{
    float absorpScale = 1.0f - s.material.roughness * 0.6f;
    return BeerLambert(ICE_ABSORPTION_COEFF * absorpScale, distance);
}

//==============================================================
// MainRayQueryCS : DXR版と同じ見た目
//==============================================================

RayTask MakeTask(float3 origin, float3 direction, float3 weight, int recursive, uint flags)
{
    RayTask task;
    task.origin = origin;
    task.direction = direction;
    task.weight = weight;
    task.recursive = recursive;
    task.flags = flags;
    return task;
}

void AddChild(inout ShadeResult result, RayTask task)
{
    result.children[result.childCount] = task;
    result.childCount++;
}

// CalculateIBLを、反射色に依存しない部分と反射色に掛かる重みに分けたもの
// CalculateIBL(...) = ambient + reflectWeight * reflectColor
void CalculateIBLSplit(float3 albedo, float3 N, float3 V, float metallic, float roughness,
    out float3 ambient, out float3 reflectWeight)
{
    float NdotV = saturate(dot(N, V));
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 F90 = max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0);
    float3 F_env = F_EnvApprox(NdotV, F0, F90);
    float3 kD_env = (1.0f - F_env) * (1.0f - metallic);
    float reflectStrength = (1.0f - roughness) * (1.0f - roughness);

    ambient = kD_env * albedo * 0.03f;
    reflectWeight = F_env * reflectStrength * lerp(float3(1.0f, 1.0f, 1.0f), albedo, metallic);
}

// MainObjectCHSと同じシェーディング
ShadeResult ShadeObject(RayTask task, HitInfo hit, int recursive, uint2 pixel, out float depth)
{
    ShadeResult result = (ShadeResult) 0;

    // DXR版と同じく、視線はカメラ位置から求める
    float3 hitPosition = task.origin + task.direction * hit.t;
    float3 cameraViewDir = normalize(gCamera.worldPosition.xyz - hitPosition);
    ObjectSurface s = LoadObjectSurface(hit, cameraViewDir);
    depth = s.depth;
    float3 viewDir = normalize(gCamera.worldPosition.xyz - s.position);

    if (!s.material.enableLighting)
    {
        result.color = s.albedo;
        return result;
    }

    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.xyz * gDirectionalLight.intensity;
    float3 directLight = CalculateBRDF(s.albedo, s.normal, viewDir, lightDir, lightColor, s.material.roughness, s.material.metallic);

    // 環境光(反射レイの色に掛かる重みを分離)
    float3 ambient, reflectWeight;
    CalculateIBLSplit(s.albedo, s.normal, viewDir, s.material.metallic, s.material.roughness, ambient, reflectWeight);

    float3 rayDir = normalize(task.direction);
    float3 reflectDir = reflect(rayDir, s.normal);

    // 透明度の表示
    if (s.type == 1)
    {
        float alpha = s.material.color.a;
        result.color = (directLight + ambient) * alpha;
        AddChild(result, MakeTask(s.position, reflectDir, reflectWeight * alpha, recursive, RAY_FLAG_NONE));

        // TranslucentRefractionと同じ屈折
        float nr = dot(s.normal, rayDir);
        float3 refracted = (nr < 0.0f)
            ? refract(rayDir, s.normal, 1.0f / s.material.ior)
            : refract(rayDir, -s.normal, s.material.ior);
        float3 refractWeight = (1.0f - alpha).xxx;
        if (length(refracted) < 0.01f)
        {
            AddChild(result, MakeTask(s.position, reflectDir, refractWeight, recursive, RAY_FLAG_NONE));
        }
        else
        {
            AddChild(result, MakeTask(s.position, refracted, refractWeight, recursive, RAY_FLAG_CULL_BACK_FACING_TRIANGLES));
        }
        return result;
    }

    // 影の色を全体に掛ける
    float3 shadowColor = ComputeShadowColor(s.position, lightDir, pixel);
    float3 shadowScale = lerp(0.5f, 1.0f, shadowColor);

    result.color = (directLight + ambient) * shadowScale;
    AddChild(result, MakeTask(s.position, reflectDir, reflectWeight * shadowScale, recursive, RAY_FLAG_NONE));
    return result;
}

// MainIceObjectCHSと同じシェーディング
ShadeResult ShadeIce(RayTask task, HitInfo hit, int recursive, out float depth)
{
    ShadeResult result = (ShadeResult) 0;

    // DXR版と同じく、視線はカメラ位置から求める
    float3 hitPosition = task.origin + task.direction * hit.t;
    IceSurface s = LoadIceSurface(hit, normalize(gCamera.worldPosition.xyz - hitPosition));
    depth = s.depth;
    float3 viewDir = normalize(gCamera.worldPosition.xyz - s.position);

    float3 worldNormal = s.normal;
    if (dot(worldNormal, viewDir) < 0.0f)
    {
        worldNormal = -worldNormal;
    }

    float3 lightDir = normalize(-gDirectionalLight.direction);
    float shadowFactor = ComputeShadowFactor(s.position, lightDir);

    // ---- IceBSDFと同じ計算。反射・屈折はレイとして積む ----
    float3 rayDir = normalize(task.direction);
    bool entering = dot(rayDir, worldNormal) < 0.0f;
    float3 N = entering ? worldNormal : -worldNormal;
    float eta = entering ? (1.0f / s.material.ior) : s.material.ior;
    float F = IceFresnel(dot(-rayDir, N), s.material.ior);

    float3 transmittance = float3(1.0f, 1.0f, 1.0f);
    if (!entering)
    {
        transmittance = IceTransmittance(s, hit.t);
    }

    // 影の中の氷の色。IceBSDFの結果全体に掛かる
    float3 shadowTint = lerp(ICE_SHADOW_TINT, float3(1.0f, 1.0f, 1.0f), shadowFactor);

    float3 refracted = refract(rayDir, N, eta);
    if (length(refracted) < 0.001f)
    {
        // 全反射
        AddChild(result, MakeTask(s.position, reflect(rayDir, N), transmittance * shadowTint, recursive, RAY_FLAG_NONE));
    }
    else
    {
        AddChild(result, MakeTask(s.position, refracted, (1.0f - F) * transmittance * s.iceColor * shadowTint, recursive, RAY_FLAG_NONE));
        AddChild(result, MakeTask(s.position, reflect(rayDir, N), F.xxx * shadowTint, recursive, RAY_FLAG_NONE));

        // 近似SSS
        if (entering && s.material.roughness > 0.01f && shadowFactor > 0.0f)
        {
            float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
            float3 sssContrib = IceFakeSSS(N, lightDir, rayDir, lightColor, s.material.roughness * 0.4f);
            result.color += sssContrib * (1.0f - F) * shadowFactor * shadowTint;
        }
    }

    result.color += IceSurfaceLighting(s, worldNormal, viewDir, shadowFactor);
    return result;
}

[numthreads(8, 8, 1)]
void MainRayQueryCS(uint3 dispatchID : SV_DispatchThreadID)
{
    uint width, height;
    gDepthOutput.GetDimensions(width, height);
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= width || pixel.y >= height)
    {
        return;
    }

    // カメラレイ(MainRayGenと同じ)
    float2 dims = float2(width, height);
    float2 d = (pixel + 0.5f) / dims * 2.0f - 1.0f;
    float3 origin = mul(float4(0, 0, 0, 1), gCamera.mtxViewInv).xyz;
    float4 target = mul(float4(d.x, -d.y, 1, 1), gCamera.mtxProjInv);
    float3 direction = normalize(mul(float4(target.xyz / target.w, 0), gCamera.mtxViewInv).xyz);

    RayTask stack[RQ_STACK_SIZE];
    int stackCount = 0;
    stack[stackCount++] = MakeTask(origin, direction, float3(1.0f, 1.0f, 1.0f), 0, RAY_FLAG_NONE);

    float3 radiance = float3(0.0f, 0.0f, 0.0f);
    float primaryDepth = 1.0f;
    bool isPrimary = true;

    for (int rayCount = 0; rayCount < RQ_MAX_RAYS && stackCount > 0; ++rayCount)
    {
        RayTask task = stack[--stackCount];

        HitInfo hit;
        if (!TraceClosest(task.origin, task.direction, task.flags, hit))
        {
            radiance += task.weight * SampleBackground(task.direction);
            isPrimary = false;
            continue;
        }

        // 再帰の上限に達したら背景を返す
        int recursive = task.recursive + 1;
        if (recursive >= RQ_MAX_RECURSIVE)
        {
            radiance += task.weight * SampleBackground(task.direction);
            continue;
        }

        float depth = 1.0f;
        ShadeResult result;
        if (hit.hitGroupIndex == HIT_GROUP_ICE)
        {
            result = ShadeIce(task, hit, recursive, depth);
        }
        else
        {
            result = ShadeObject(task, hit, recursive, pixel, depth);
        }

        if (isPrimary)
        {
            primaryDepth = depth;
            isPrimary = false;
        }

        radiance += task.weight * result.color;

        // 子のレイを積む
        for (int i = 0; i < result.childCount; ++i)
        {
            if (stackCount < RQ_STACK_SIZE)
            {
                RayTask child = result.children[i];
                child.weight *= task.weight;
                stack[stackCount++] = child;
            }
        }
    }

    gOutput[pixel] = float4(radiance, 1.0f);
    gDepthOutput[pixel] = primaryDepth;
}

//==============================================================
// MainPathTraceCS : パストレーシング
//==============================================================

// PCGハッシュによる乱数
uint PcgHash(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float NextRandom(inout uint rng)
{
    rng = PcgHash(rng);
    return float(rng) / 4294967296.0f;
}

float Luminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

// 寄与の明るさを上限で抑える
float3 ClampLuminance(float3 color, float maxLuminance)
{
    float lum = Luminance(color);
    return (lum > maxLuminance) ? color * (maxLuminance / lum) : color;
}

// 法線を軸にした正規直交基底
void BuildBasis(float3 n, out float3 t, out float3 b)
{
    float3 up = abs(n.y) < 0.999f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

// コサイン重み付きの半球サンプリング。pdf = cos / PI
float3 SampleCosineHemisphere(float3 n, float2 u)
{
    float r = sqrt(u.x);
    float phi = 2.0f * PI * u.y;
    float3 t, b;
    BuildBasis(n, t, b);
    return normalize(t * (r * cos(phi)) + b * (r * sin(phi)) + n * sqrt(saturate(1.0f - u.x)));
}

// GGXの可視法線分布(VNDF)からマイクロファセット法線をサンプリングする [Heitz 2018]
float3 SampleGGXVNDF(float3 n, float3 v, float alpha, float2 u)
{
    float3 t, b;
    BuildBasis(n, t, b);
    float3 vLocal = float3(dot(v, t), dot(v, b), dot(v, n));

    float3 vh = normalize(float3(alpha * vLocal.x, alpha * vLocal.y, vLocal.z));
    float lensq = vh.x * vh.x + vh.y * vh.y;
    float3 T1 = lensq > 0.0f ? float3(-vh.y, vh.x, 0.0f) * rsqrt(lensq) : float3(1.0f, 0.0f, 0.0f);
    float3 T2 = cross(vh, T1);
    float r = sqrt(u.x);
    float phi = 2.0f * PI * u.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + vh.z);
    t2 = (1.0f - s) * sqrt(saturate(1.0f - t1 * t1)) + s * t2;
    float3 nh = t1 * T1 + t2 * T2 + sqrt(saturate(1.0f - t1 * t1 - t2 * t2)) * vh;
    float3 hLocal = normalize(float3(alpha * nh.x, alpha * nh.y, max(0.0f, nh.z)));

    return normalize(t * hLocal.x + b * hLocal.y + n * hLocal.z);
}

// GGXのSmithマスキング関数 G1
float SmithG1GGX(float NdotX, float alpha)
{
    float a2 = alpha * alpha;
    return 2.0f * NdotX / (NdotX + sqrt(a2 + (1.0f - a2) * NdotX * NdotX));
}

// 通常オブジェクトのBSDFから次の方向をサンプリングする
// weight : BSDF * cos / pdf
bool SampleObjectBSDF(ObjectSurface s, float3 V, inout uint rng, out float3 direction, out float3 weight)
{
    float3 N = s.normal;
    float NdotV = saturate(dot(N, V));
    float metallic = s.material.metallic;
    float alpha = max(s.material.roughness * s.material.roughness, 0.002f);
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), s.albedo, metallic);

    // 鏡面反射と拡散反射のどちらを選ぶか。フレネルの強さで確率を決める
    float3 Fv = F_Schlick(NdotV, F0);
    float specularProb = clamp(Luminance(Fv) + metallic * (1.0f - Luminance(Fv)), 0.1f, 0.9f);

    float2 u = float2(NextRandom(rng), NextRandom(rng));
    if (NextRandom(rng) < specularProb)
    {
        float3 H = SampleGGXVNDF(N, V, alpha, u);
        direction = reflect(-V, H);
        float NdotL = dot(N, direction);
        if (NdotL <= 0.0f)
        {
            return false;
        }
        // VNDFサンプリングでは重み = F * G1(L)
        float3 F = F_Schlick(saturate(dot(V, H)), F0);
        weight = F * SmithG1GGX(NdotL, alpha) / specularProb;
    }
    else
    {
        direction = SampleCosineHemisphere(N, u);
        // コサインサンプリングでは重み = albedo
        weight = s.albedo * (1.0f - Fv) * (1.0f - metallic) / (1.0f - specularProb);
    }
    return true;
}

// 3x3行列の逆行列
float3x3 Inverse3x3(float3x3 m)
{
    float3 r0 = cross(m[1], m[2]);
    float3 r1 = cross(m[2], m[0]);
    float3 r2 = cross(m[0], m[1]);
    float invDet = 1.0f / dot(m[0], r0);
    return transpose(float3x3(r0, r1, r2)) * invDet;
}

// ビジビリティバッファの値から、RayQueryと同じヒット情報を作る
// 重心座標と距離は、カメラレイと三角形の交差から求める
bool HitFromVisibility(uint2 visibility, float3 origin, float3 direction, out HitInfo hit)
{
    hit = (HitInfo) 0;
    if (visibility.x == 0)
    {
        return false;
    }

    // D3D12_RAYTRACING_INSTANCE_DESCは64byte。Transform(3x4) → InstanceID(24bit) → ヒットグループ(24bit)
    uint base = (visibility.x - 1) * 64;
    float4 row0 = asfloat(gInstanceDescs.Load4(base + 0));
    float4 row1 = asfloat(gInstanceDescs.Load4(base + 16));
    float4 row2 = asfloat(gInstanceDescs.Load4(base + 32));
    hit.instanceID = gInstanceDescs.Load(base + 48) & 0x00FFFFFF;
    hit.hitGroupIndex = gInstanceDescs.Load(base + 52) & 0x00FFFFFF;
    hit.primitiveIndex = visibility.y;

    // RayQueryのCommittedObjectToWorld4x3と同じ形にする
    float3x4 objectToWorld = float3x4(row0, row1, row2);
    hit.objectToWorld = transpose(objectToWorld);
    float3x3 inverseRotation = Inverse3x3((float3x3) objectToWorld);
    float3 inverseTranslate = -mul(inverseRotation, float3(row0.w, row1.w, row2.w));
    hit.worldToObject = transpose(float3x4(
        float4(inverseRotation[0], inverseTranslate.x),
        float4(inverseRotation[1], inverseTranslate.y),
        float4(inverseRotation[2], inverseTranslate.z)));

    // LoadHitVertexと同じ規則で三角形の頂点を読む
    BufferRef ref = gBufferRefs[hit.instanceID];
    bool applyOffset = hit.hitGroupIndex == HIT_GROUP_ICE;
    uint indexOffset = applyOffset ? ref.indexOffset : 0;
    uint vertexOffset = applyOffset ? ref.vertexOffset : 0;
    float3 positions[3];
    for (int i = 0; i < 3; ++i)
    {
        uint localIndex = gBufferData[ref.indexHandle].Load<uint>((hit.primitiveIndex * 3 + i) * 4 + indexOffset * 4);
        float3 localPosition = gBufferData[ref.vertexHandle].Load<float3>((localIndex + vertexOffset) * VERTEX_STRIDE);
        positions[i] = mul(float4(localPosition, 1.0f), hit.objectToWorld);
    }

    // Möller–Trumboreでレイと三角形の平面との交点を求める
    // 画素内のジッターで三角形を少しはみ出すことがあるため、重心座標の範囲は判定しない
    float3 e1 = positions[1] - positions[0];
    float3 e2 = positions[2] - positions[0];
    float3 pvec = cross(direction, e2);
    float det = dot(e1, pvec);
    if (abs(det) < 1e-12f)
    {
        return false;
    }
    float invDet = 1.0f / det;
    float3 tvec = origin - positions[0];
    float3 qvec = cross(tvec, e1);
    hit.barys = float2(dot(tvec, pvec), dot(direction, qvec)) * invDet;
    hit.t = dot(e2, qvec) * invDet;
    return hit.t > 0.0f;
}

// 法線の向きにずらしてレイの始点にする。自己交差を防ぐ
float3 OffsetRayOrigin(float3 position, float3 normal, float3 direction)
{
    return position + normal * (dot(direction, normal) > 0.0f ? 0.001f : -0.001f);
}

[numthreads(8, 8, 1)]
void MainPathTraceCS(uint3 dispatchID : SV_DispatchThreadID)
{
    uint width, height;
    gDepthOutput.GetDimensions(width, height);
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= width || pixel.y >= height)
    {
        return;
    }

    uint rng = PcgHash(pixel.x + pixel.y * width + PcgHash(gFrameIndex));

    // カメラレイ。画素内でジッターをかける
    float2 dims = float2(width, height);
    float2 jitter = float2(NextRandom(rng), NextRandom(rng));
    float2 d = (pixel + jitter) / dims * 2.0f - 1.0f;
    float3 origin = mul(float4(0, 0, 0, 1), gCamera.mtxViewInv).xyz;
    float4 target = mul(float4(d.x, -d.y, 1, 1), gCamera.mtxProjInv);
    float3 direction = normalize(mul(float4(target.xyz / target.w, 0), gCamera.mtxViewInv).xyz);

    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;

    float3 radiance = float3(0.0f, 0.0f, 0.0f);
    float3 throughput = float3(1.0f, 1.0f, 1.0f);
    float primaryDepth = 1.0f;
    // 最初のヒットの情報。背景なら距離0、アルベド1のまま
    float4 primaryGuide = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 primaryAlbedo = float3(1.0f, 1.0f, 1.0f);

    // パスが上限より前に終わったか
    bool pathEnded = false;
    for (int bounce = 0; bounce < PT_MAX_BOUNCE; ++bounce)
    {
        // 2バウンス目以降はホタル状のノイズを抑える
        float maxLuminance = (bounce == 0) ? 1e30f : PT_MAX_INDIRECT_LUMINANCE;

        // 最初のヒットはビジビリティバッファから、2バウンス目以降はレイを飛ばして求める
        HitInfo hit;
        bool isHit = (bounce == 0 && gUseVisibilityBuffer != 0)
            ? HitFromVisibility(gVisibilityBuffer[pixel], origin, direction, hit)
            : TraceClosest(origin, direction, RAY_FLAG_NONE, hit);
        if (!isHit)
        {
            // 環境光
            radiance += ClampLuminance(throughput * SampleBackground(direction), maxLuminance);
            pathEnded = true;
            break;
        }

        float3 V = -direction;

        if (hit.hitGroupIndex == HIT_GROUP_ICE)
        {
            IceSurface s = LoadIceSurface(hit, V);
            if (bounce == 0)
            {
                primaryDepth = s.depth;
                primaryGuide = float4(dot(direction, s.normal) < 0.0f ? s.normal : -s.normal, hit.t);
            }

            // 幾何的な表裏で氷に入るか出るかを決める
            bool entering = dot(direction, s.normal) < 0.0f;
            float3 N = entering ? s.normal : -s.normal;

            // 氷の内部を進んだ分だけ吸収する
            if (!entering)
            {
                throughput *= IceTransmittance(s, hit.t);
            }

            // 表面のハイライトなど、光源から直接届く成分
            if (entering)
            {
                float shadowFactor = ComputeShadowFactor(s.position, lightDir);
                radiance += ClampLuminance(throughput * IceSurfaceLighting(s, N, V, shadowFactor), maxLuminance);
            }

            // 粗い氷はマイクロファセット法線をばらつかせる
            float3 M = N;
            if (s.material.roughness > 0.01f)
            {
                float alpha = s.material.roughness * s.material.roughness;
                M = SampleGGXVNDF(N, V, alpha, float2(NextRandom(rng), NextRandom(rng)));
            }

            // フレネルの確率で反射か屈折かを選ぶ。選んだ確率で割るため重みは1になる
            float eta = entering ? (1.0f / s.material.ior) : s.material.ior;
            float F = IceFresnel(dot(V, M), s.material.ior);
            float3 refracted = refract(direction, M, eta);
            if (dot(refracted, refracted) < 1e-6f || NextRandom(rng) < F)
            {
                direction = reflect(direction, M);
            }
            else
            {
                direction = normalize(refracted);
                throughput *= s.iceColor;
            }
            origin = OffsetRayOrigin(s.position, N, direction);
        }
        else
        {
            ObjectSurface s = LoadObjectSurface(hit, V);
            if (bounce == 0)
            {
                primaryDepth = s.depth;
                primaryGuide = float4(s.normal, hit.t);
                primaryAlbedo = s.albedo;
            }

            if (!s.material.enableLighting)
            {
                radiance += ClampLuminance(throughput * s.albedo, maxLuminance);
                pathEnded = true;
                break;
            }

            // 透明なオブジェクトは、不透明度の確率で表面か透過かを選ぶ
            if (s.type == 1 && NextRandom(rng) >= s.material.color.a)
            {
                bool frontFace = dot(direction, s.normal) < 0.0f;
                float3 refracted = frontFace
                    ? refract(direction, s.normal, 1.0f / s.material.ior)
                    : refract(direction, -s.normal, s.material.ior);
                direction = (dot(refracted, refracted) < 1e-6f) ? reflect(direction, s.normal) : normalize(refracted);
                origin = OffsetRayOrigin(s.position, s.normal, direction);
                continue;
            }

            // 直接光(NEE)。1バウンス目だけ氷のコースティクス影を使い、以降は軽い影にする
            float3 visibility = (bounce == 0)
                ? ComputeShadowColor(s.position, lightDir, pixel)
                : ComputeShadowFactor(s.position, lightDir).xxx;
            float3 directLight = CalculateBRDF(s.albedo, s.normal, V, lightDir, lightColor, s.material.roughness, s.material.metallic);
            radiance += ClampLuminance(throughput * directLight * visibility, maxLuminance);

            // 次の方向をBSDFからサンプリング
            float3 weight;
            if (!SampleObjectBSDF(s, V, rng, direction, weight))
            {
                pathEnded = true;
                break;
            }
            throughput *= weight;
            origin = OffsetRayOrigin(s.position, s.normal, direction);
        }

        // ロシアンルーレット。寄与の小さいパスを確率的に打ち切る
        if (bounce >= PT_RR_START_BOUNCE)
        {
            float survive = saturate(max(throughput.r, max(throughput.g, throughput.b)));
            if (NextRandom(rng) >= survive)
            {
                pathEnded = true;
                break;
            }
            throughput /= survive;
        }
    }

    // 上限まで追跡したパスは、DXR版の再帰上限と同じく背景の色で終える。氷の内部で反射し続けたパスが黒くなるのを防ぐ
    if (!pathEnded)
    {
        radiance += ClampLuminance(throughput * SampleBackground(direction), PT_MAX_INDIRECT_LUMINANCE);
    }

    gOutput[pixel] = float4(radiance, 1.0f);
    gDepthOutput[pixel] = primaryDepth;
    gGuideOutput[pixel] = primaryGuide;
    gAlbedoOutput[pixel] = float4(primaryAlbedo, 1.0f);
}
