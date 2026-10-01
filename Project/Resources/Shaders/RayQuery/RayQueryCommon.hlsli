#ifndef RAYQUERY_COMMON_HLSLI
#define RAYQUERY_COMMON_HLSLI
#include "../LightElement.hlsli"

// RayQuery(インラインレイトレーシング)で使用する共通処理
// DXR専用の組み込み関数(WorldRayDirection, InstanceIDなど)は使わず、ヒット情報は引数で受け渡す

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
    float4x4 mtxViewInv; // ビュー逆行列
    float4x4 mtxProjInv; // プロジェクション逆行列
};

struct BufferRef
{
    uint type; // バッファデータのタイプ
    uint MaterialIndex; // マテリアルデータの参照するハンドル

    uint indexHandle; // モデルのインデックス
    uint vertexHandle; // モデルの頂点

    uint vertexOffset;
    uint indexOffset;
    float2 pad;
};

struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 tangent;
};

// Root Signature
RWTexture2D<float4> gOutput : register(u0);
RWTexture2D<float> gDepthOutput : register(u1);
RaytracingAccelerationStructure gRtScene : register(t0, space0);
Texture2D<float4> gTexture[] : register(t0, space1);
StructuredBuffer<BufferRef> gBufferRefs : register(t0, space2);
ByteAddressBuffer gBufferData[] : register(t0, space3);
TextureCube<float4> gBackgroundTexture : register(t1, space0);
SamplerState gSampler : register(s0);

ConstantBuffer<Camera> gCamera : register(b0);
cbuffer LightGroup : register(b1)
{
    DirectionalLight gDirectionalLight;
    PointLight gPointLight;
    SpotLight gSpotLight;
    uint environmentTexture;
    int isActiveEnvironment;
};

// マテリアルグラフのテクスチャサンプリング。CSでは微分が使えないためSampleLevelを使う
#define MATERIAL_SAMPLE(tex, uv) (tex).SampleLevel(gSampler, (uv), 0)

// レイのフィルタリングに使うインスタンスマスク
#define RAY_MASK_OPAQUE 0x01 // 不透明。影レイを完全に遮る
#define RAY_MASK_ICE    0x02 // 氷などの透過物。影レイを一部だけ遮る
#define RAY_MASK_ALL    0xFF // 全てのインスタンス

// マテリアルID。TLASのInstanceContributionToHitGroupIndexと対応する
#define MATERIAL_ID_DEFAULT 0
#define MATERIAL_ID_ICE     1

// レイの範囲
static const float RAY_T_MIN = 0.001f;
static const float RAY_T_MAX = 100000.0f;

// レイの最大深度。この深度に達したレイは背景色を返す
static const int MAX_RAY_DEPTH = 4;

// 氷が落とす影の濃さ
static const float ICE_SHADOW_DENSITY = 0.25f;

// 頂点データのストライド
static const uint VERTEX_STRIDE = 52;

// ヒット情報。CHSで組み込み関数から取得していた値をまとめたもの
struct HitInfo
{
    bool isHit;
    float t; // RayTCurrent()
    uint instanceID; // InstanceID()
    uint primitiveIndex; // PrimitiveIndex()
    uint materialId; // ヒットグループのインデックス
    float2 barys; // 重心座標
    float4x3 objectToWorld; // ObjectToWorld4x3()
    float4x3 worldToObject; // WorldToObject4x3()
};

// マテリアルのシェーディング結果
// 再帰でレイを飛ばす代わりに、二次レイの方向と重みを返す
struct ShadeResult
{
    float3 position; // ヒット位置(ワールド)
    float3 color; // このヒット点で確定した色

    bool hasReflection; // 反射レイの有無
    float3 reflectionDir;
    float3 reflectionWeight;

    bool hasRefraction; // 屈折レイの有無
    float3 refractionDir;
    float3 refractionWeight;
    uint refractionFlags;
};

ShadeResult InitShadeResult()
{
    ShadeResult result;
    result.position = float3(0.0f, 0.0f, 0.0f);
    result.color = float3(0.0f, 0.0f, 0.0f);
    result.hasReflection = false;
    result.reflectionDir = float3(0.0f, 0.0f, 0.0f);
    result.reflectionWeight = float3(0.0f, 0.0f, 0.0f);
    result.hasRefraction = false;
    result.refractionDir = float3(0.0f, 0.0f, 0.0f);
    result.refractionWeight = float3(0.0f, 0.0f, 0.0f);
    result.refractionFlags = RAY_FLAG_NONE;
    return result;
}

void SetReflection(inout ShadeResult result, float3 dir, float3 weight)
{
    result.hasReflection = true;
    result.reflectionDir = dir;
    result.reflectionWeight = weight;
}

void SetRefraction(inout ShadeResult result, float3 dir, float3 weight, uint flags)
{
    result.hasRefraction = true;
    result.refractionDir = dir;
    result.refractionWeight = weight;
    result.refractionFlags = flags;
}

RayDesc MakeRay(float3 origin, float3 direction)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = RAY_T_MIN;
    ray.TMax = RAY_T_MAX;
    return ray;
}

// 背景色を取得
float3 SampleBackground(float3 direction)
{
    return gBackgroundTexture.SampleLevel(gSampler, direction, 0.0f).rgb;
}

// 最も近いヒットを探す
HitInfo TraceClosestHit(RayDesc ray, uint rayFlags, uint rayMask)
{
    RayQuery<RAY_FLAG_NONE> q;
    q.TraceRayInline(gRtScene, rayFlags, rayMask, ray);

    // 非Opaqueの三角形はAnyHitが無い時と同様に全て受け入れる
    while (q.Proceed())
    {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            q.CommitNonOpaqueTriangleHit();
        }
    }

    HitInfo hit = (HitInfo) 0;
    hit.isHit = (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT);
    if (hit.isHit)
    {
        hit.t = q.CommittedRayT();
        hit.instanceID = q.CommittedInstanceID();
        hit.primitiveIndex = q.CommittedPrimitiveIndex();
        hit.materialId = q.CommittedInstanceContributionToHitGroupIndex() + q.CommittedGeometryIndex();
        hit.barys = q.CommittedTriangleBarycentrics();
        hit.objectToWorld = q.CommittedObjectToWorld4x3();
        hit.worldToObject = q.CommittedWorldToObject4x3();
    }
    return hit;
}

// 影判定用のレイ
// rayMask : 遮蔽物として扱うインスタンスのマスク
bool ShootShadowRay(float3 origin, float3 direction, uint rayMask = RAY_MASK_ALL)
{
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
    q.TraceRayInline(gRtScene, RAY_FLAG_NONE, rayMask, MakeRay(origin, direction));

    while (q.Proceed())
    {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            q.CommitNonOpaqueTriangleHit();
        }
    }
    return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

// 遮蔽物の種類を区別して影の透過率を求める
float ComputeShadowFactor(float3 origin, float3 direction)
{
    // 不透明な遮蔽物は光を完全に遮る
    if (ShootShadowRay(origin, direction, RAY_MASK_OPAQUE))
    {
        return 0.0f;
    }

    // 氷は光を透過するため、薄い影にとどめる
    if (ShootShadowRay(origin, direction, RAY_MASK_ICE))
    {
        return 1.0f - ICE_SHADOW_DENSITY;
    }

    return 1.0f;
}

inline float2 CalcHitAttribute2(float2 vertexAttribute[3], float2 barycentrics)
{
    float2 ret;
    ret = vertexAttribute[0];
    ret += barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]);
    ret += barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
    return ret;
}

float3 CalcHitAttribute3(float3 vertexAttribute[3], float2 barycentrics)
{
    float3 ret;
    ret = vertexAttribute[0];
    ret += barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]);
    ret += barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
    return ret;
}

float4 CalcHitAttribute4(float4 vertexAttribute[3], float2 barycentrics)
{
    float4 ret;
    ret = vertexAttribute[0];
    ret += barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]);
    ret += barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
    return ret;
}

// ヒットした三角形の頂点データを補間して取得する
VertexData FetchHitVertex(HitInfo hit, uint vertexHandle, uint indexHandle, uint vertexOffset, uint indexOffset)
{
    uint start = hit.primitiveIndex * 3;

    float3 positions[3];
    float2 texcoords[3];
    float3 normals[3];
    float4 tangents[3];

    // スレッドごとに参照するバッファが異なるためNonUniformResourceIndexを使う
    ByteAddressBuffer indexBuffer = gBufferData[NonUniformResourceIndex(indexHandle)];
    ByteAddressBuffer vertexBuffer = gBufferData[NonUniformResourceIndex(vertexHandle)];

    for (int i = 0; i < 3; ++i)
    {
        uint localIndex = indexBuffer.Load<uint>((start + i) * 4 + indexOffset * 4);
        uint index = localIndex + vertexOffset;
        VertexData v = vertexBuffer.Load<VertexData>(index * VERTEX_STRIDE);

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

// テクスチャをサンプリング
float4 SampleMaterialTexture(uint textureHandle, float2 uv)
{
    return gTexture[NonUniformResourceIndex(textureHandle)].SampleLevel(gSampler, uv, 0);
}

#endif
