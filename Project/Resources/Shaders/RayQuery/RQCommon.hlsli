#ifndef RQ_COMMON_HLSLI
#define RQ_COMMON_HLSLI
#include "../LightElement.hlsli"

// マテリアルの種類
#define RQ_MATERIAL_DEFAULT 0
#define RQ_MATERIAL_ICE     1
#define RQ_MATERIAL_COUNT   2

// スレッドグループサイズ
#define RQ_TRACE_GROUP_SIZE 8
#define RQ_SHADE_GROUP_SIZE 64

// レイのフィルタリングに使うインスタンスマスク
#define RAY_MASK_OPAQUE 0x01 // 不透明。影レイを完全に遮る
#define RAY_MASK_ICE    0x02 // 氷などの透過物。影レイを一部だけ遮る
#define RAY_MASK_ALL    0xFF // 全てのインスタンス

// 氷が落とす影の濃さ
static const float ICE_SHADOW_DENSITY = 0.25f;
// 氷のBeer-Lambert則の吸収係数
static const float3 ICE_ABSORPTION_COEFF = float3(0.80f, 0.25f, 0.04f);
// 影の中の氷にかける色
static const float3 ICE_SHADOW_TINT = float3(0.80f, 0.87f, 1.00f);

// この段数に達したヒットは背景で代用する
static const int RQ_MAX_RECURSIVE = 4;

static const float RQ_TMIN = 0.001f;
static const float RQ_TMAX = 100000.0f;

// カメラ
struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
    float4x4 mtxViewInv; // ビュー逆行列
    float4x4 mtxProjInv; // プロジェクション逆行列
};

// 参照用
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

// ResourceDescriptorHeapで参照するリソースのインデックス一覧
struct RQIndices
{
    uint tlas;         // TLAS
    uint bufferRefs;   // BufferRefのStructuredBuffer
    uint output;       // 出力画像
    uint depthOutput;  // 出力深度
    uint skybox;       // 背景のキューブマップ
    uint hitBuffer;    // 1次レイのヒット情報
    uint pixelList;    // マテリアルごとのピクセルリスト
    uint counter;      // マテリアルごとのピクセル数
    uint dispatchArgs; // マテリアルごとのDispatch引数
    uint width;
    uint height;

    // RTAO
    uint frameIndex;        // 乱数の種に使うフレーム番号
    uint aoHistoryCurrent;  // 今フレームのAO履歴
    uint aoHistoryPrevious; // 前フレームのAO履歴
    uint aoOutput;          // ノイズ除去後のAO
    uint prevCamera;        // 前フレームのカメラ
    uint aoEnabled;         // AOを使用するか
    uint aoSampleCount;     // 1ピクセルあたりのAOレイの本数
    float aoRadius;         // 遮蔽物を探す距離
    float aoStrength;       // AOの強さ
    float ambientIntensity; // 環境光の強さ
    uint aoDebugView;       // AOだけを表示するか
    uint aoHistoryClamp;    // 履歴を周囲の値の範囲に収めるか
    float aoClampGamma;     // 収める範囲の広さ

    // テクスチャのLOD
    uint texLodEnabled;     // LODを計算するか
    float texLodBias;       // ミップレベルに足す値
};

// 1次レイのヒット情報
struct RQHit
{
    uint instanceIndex;  // TLAS内のインスタンス番号
    uint instanceID;     // BufferRefのインデックス
    uint primitiveIndex;
    float2 barys;
    float t;
    uint material;       // マテリアル番号
    uint pad;
};

// ミスしたピクセルのinstanceID
static const uint RQ_INVALID_ID = 0xFFFFFFFF;

// AOの履歴
struct RQAOSample
{
    float3 normal;        // ワールド法線
    float depth;          // カメラからの距離
    float ao;             // AO
    float historyLength;  // 蓄積したフレーム数
    float rawAO;          // ノイズ除去前のこのフレームのAO
    float pad;
};

// 前フレームのカメラ
struct RQPrevCamera
{
    float4x4 vpMatrix;
    float3 worldPosition;
    uint valid;
};

struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 tangent;
};
static const uint VERTEX_STRIDE = 52;

// 通常マテリアル
struct MaterialData
{
    float4 color;

    int enableLighting;
    float dissolveThreshold;
    float2 padding0;

    float4x4 uvTransform;

    float4 specularColor;

    float shininess;
    uint textureHandle;
    float metallic;
    int isActiveShadow;

    float ior;
    float roughness;
    uint normalTextureHandle;
    uint dissolveTextureHandle;
};

// 氷のマテリアル
struct IceMaterialData
{
    float4 color;

    int enableLighting;
    float dissolveThreshold;
    float2 padding0;

    float4x4 uvTransform;

    float4 specularColor;

    float shininess;
    uint textureHandle;
    float metallic;
    int isActiveShadow;

    float ior;
    float roughness;
    uint normalTextureHandle;
    uint dissolveTextureHandle;

    float chipScale;
    float chipStrength;
    float edgeWidth;
    float edgeStrength;

    float microScale;
    float microStrength;
    uint heightTextureHandle;
    float heightScale;

    float bubbleScale;
    float bubbleMaxDepth;
    float bubbleDensity;
    float bubbleJitter;

    float bubbleHighlight;
    float rimIntensity;
    float rimPower;
    float1 padding1;

    float4 rimColor;
};

// ルート引数
ConstantBuffer<RQIndices> gIndices : register(b2);
ConstantBuffer<Camera> gCamera : register(b0);
cbuffer LightGroup : register(b1)
{
    DirectionalLight gDirectionalLight;
    PointLight gPointLight;
    SpotLight gSpotLight;
    uint environmentTexture;
    int isActiveEnvironment;
};
// TLASのインスタンス情報
ByteAddressBuffer gInstanceDescs : register(t0);
SamplerState gSampler : register(s0);

// ============================================================
// リソースの取得
// ============================================================

RaytracingAccelerationStructure GetScene() { return ResourceDescriptorHeap[gIndices.tlas]; }
StructuredBuffer<BufferRef> GetBufferRefs() { return ResourceDescriptorHeap[gIndices.bufferRefs]; }
RWTexture2D<float4> GetOutput() { return ResourceDescriptorHeap[gIndices.output]; }
RWTexture2D<float> GetDepthOutput() { return ResourceDescriptorHeap[gIndices.depthOutput]; }
TextureCube<float4> GetBackgroundTexture() { return ResourceDescriptorHeap[gIndices.skybox]; }
RWStructuredBuffer<RQHit> GetHitBuffer() { return ResourceDescriptorHeap[gIndices.hitBuffer]; }
RWStructuredBuffer<uint> GetPixelList() { return ResourceDescriptorHeap[gIndices.pixelList]; }
RWStructuredBuffer<uint> GetCounter() { return ResourceDescriptorHeap[gIndices.counter]; }
RWStructuredBuffer<uint3> GetDispatchArgs() { return ResourceDescriptorHeap[gIndices.dispatchArgs]; }
RWStructuredBuffer<RQAOSample> GetAOHistoryCurrent() { return ResourceDescriptorHeap[gIndices.aoHistoryCurrent]; }
RWStructuredBuffer<RQAOSample> GetAOHistoryPrevious() { return ResourceDescriptorHeap[gIndices.aoHistoryPrevious]; }
RWStructuredBuffer<float> GetAOOutput() { return ResourceDescriptorHeap[gIndices.aoOutput]; }
RWStructuredBuffer<RQPrevCamera> GetPrevCamera() { return ResourceDescriptorHeap[gIndices.prevCamera]; }

Texture2D<float4> GetTexture(uint index)
{
    return ResourceDescriptorHeap[NonUniformResourceIndex(index)];
}

ByteAddressBuffer GetBuffer(uint index)
{
    return ResourceDescriptorHeap[NonUniformResourceIndex(index)];
}

uint GetPixelCount() { return gIndices.width * gIndices.height; }

// ============================================================
// 数学ヘルパー
// ============================================================

inline float3 CalcHitAttribute3(float3 a[3], float2 barys)
{
    return a[0] + barys.x * (a[1] - a[0]) + barys.y * (a[2] - a[0]);
}

inline float2 CalcHitAttribute2(float2 a[3], float2 barys)
{
    return a[0] + barys.x * (a[1] - a[0]) + barys.y * (a[2] - a[0]);
}

inline float4 CalcHitAttribute4(float4 a[3], float2 barys)
{
    return a[0] + barys.x * (a[1] - a[0]) + barys.y * (a[2] - a[0]);
}

float3x3 Inverse3x3(float3x3 m)
{
    float3 c0 = cross(m[1], m[2]);
    float3 c1 = cross(m[2], m[0]);
    float3 c2 = cross(m[0], m[1]);
    float det = dot(m[0], c0);
    return transpose(float3x3(c0, c1, c2)) / det;
}

// オブジェクト->ワールドの行列をインスタンス情報から読む
float3x4 LoadObjectToWorld(uint instanceIndex)
{
    uint base = instanceIndex * 64;
    float4 r0 = asfloat(gInstanceDescs.Load4(base + 0));
    float4 r1 = asfloat(gInstanceDescs.Load4(base + 16));
    float4 r2 = asfloat(gInstanceDescs.Load4(base + 32));
    return float3x4(r0, r1, r2);
}

// ============================================================
// レイ
// ============================================================

// カメラからの1次レイを作る
RayDesc GeneratePrimaryRay(uint2 pixel)
{
    float2 dims = float2(gIndices.width, gIndices.height);
    float2 d = (pixel + 0.5f) / dims * 2.0f - 1.0f;

    RayDesc ray;
    ray.Origin = mul(float4(0, 0, 0, 1), gCamera.mtxViewInv).xyz;
    float4 target = mul(float4(d.x, -d.y, 1, 1), gCamera.mtxProjInv);
    ray.Direction = normalize(mul(float4(target.xyz / target.w, 0), gCamera.mtxViewInv).xyz);
    ray.TMin = RQ_TMIN;
    ray.TMax = RQ_TMAX;
    return ray;
}

RayDesc MakeRay(float3 origin, float3 direction)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = RQ_TMIN;
    ray.TMax = RQ_TMAX;
    return ray;
}

// 最も近いヒットを探す
bool TraceClosest(RayDesc ray, uint rayFlags, uint rayMask, out RQHit hit, out uint material)
{
    hit = (RQHit) 0;
    material = RQ_MATERIAL_DEFAULT;

    RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;
    q.TraceRayInline(GetScene(), rayFlags, rayMask, ray);
    q.Proceed();

    if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
    {
        return false;
    }

    hit.instanceIndex = q.CommittedInstanceIndex();
    hit.instanceID = q.CommittedInstanceID();
    hit.primitiveIndex = q.CommittedPrimitiveIndex();
    hit.barys = q.CommittedTriangleBarycentrics();
    hit.t = q.CommittedRayT();
    // DXRのヒットグループ番号をそのままマテリアル番号として使う
    material = min(q.CommittedInstanceContributionToHitGroupIndex(), RQ_MATERIAL_COUNT - 1);
    hit.material = material;
    return true;
}

// 影判定
bool ShadowRayHit(float3 origin, float3 direction, uint rayMask)
{
    RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
    q.TraceRayInline(GetScene(), RAY_FLAG_NONE, rayMask, MakeRay(origin, direction));
    q.Proceed();
    return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

// 遮蔽物の種類を区別して影の透過率を求める
float ComputeShadowFactor(float3 origin, float3 direction)
{
    // 不透明な遮蔽物は光を完全に遮る
    if (ShadowRayHit(origin, direction, RAY_MASK_OPAQUE))
    {
        return 0.0f;
    }
    // 氷は光を透過するため、薄い影に
    if (ShadowRayHit(origin, direction, RAY_MASK_ICE))
    {
        return 1.0f - ICE_SHADOW_DENSITY;
    }
    return 1.0f;
}

inline float3 SampleEnvironment(float3 direction)
{
    return GetBackgroundTexture().SampleLevel(gSampler, direction, 0.0).rgb;
}

// ============================================================
// ヒット位置の情報
// ============================================================

struct SurfaceHit
{
    VertexData vtx;
    float3x4 objectToWorld;
    float3 worldPosition;
    BufferRef ref;
};

VertexData FetchHitVertex(BufferRef ref, uint primitiveIndex, float2 barys)
{
    uint start = primitiveIndex * 3;
    ByteAddressBuffer indexBuffer = GetBuffer(ref.indexHandle);
    ByteAddressBuffer vertexBuffer = GetBuffer(ref.vertexHandle);

    float3 positions[3];
    float2 texcoords[3];
    float3 normals[3];
    float4 tangents[3];

    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        uint index = indexBuffer.Load<uint>((start + i + ref.indexOffset) * 4) + ref.vertexOffset;
        VertexData v = vertexBuffer.Load<VertexData>(index * VERTEX_STRIDE);
        positions[i] = v.position.xyz;
        normals[i] = v.normal;
        texcoords[i] = v.texcoord;
        tangents[i] = v.tangent;
    }

    VertexData v = (VertexData) 0;
    v.position = float4(CalcHitAttribute3(positions, barys), 1.0f);
    v.texcoord = CalcHitAttribute2(texcoords, barys);
    v.normal = normalize(CalcHitAttribute3(normals, barys));
    v.tangent = CalcHitAttribute4(tangents, barys);
    return v;
}

SurfaceHit LoadSurfaceHit(RQHit hit)
{
    SurfaceHit s;
    s.ref = GetBufferRefs()[hit.instanceID];
    s.vtx = FetchHitVertex(s.ref, hit.primitiveIndex, hit.barys);
    s.objectToWorld = LoadObjectToWorld(hit.instanceIndex);
    s.worldPosition = mul(s.objectToWorld, s.vtx.position);
    return s;
}

// オブジェクト空間の法線をワールド空間へ
float3 ObjectNormalToWorld(float3 localNormal, float3x4 objectToWorld)
{
    float3x3 worldToObject = Inverse3x3((float3x3) objectToWorld);
    return normalize(mul(localNormal, worldToObject));
}

float WriteDepthFromWorld(float3 worldPosition)
{
    float4 clipPos = mul(float4(worldPosition, 1.0f), gCamera.vpMatrix);
    return clipPos.z / clipPos.w;
}

// ============================================================
// 環境光とAO
// ============================================================

// 背景のキューブマップから受ける拡散光の近似
float3 EnvironmentDiffuse(float3 N)
{
    float3 up = abs(N.y) < 0.999f ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);

    static const uint kCount = 8;
    float3 sum = float3(0, 0, 0);
    [unroll]
    for (uint i = 0; i < kCount; ++i)
    {
        float u = (i + 0.5f) / kCount;
        float r = sqrt(u);
        float phi = i * 2.39996323f;
        float3 dir = T * (r * cos(phi)) + B * (r * sin(phi)) + N * sqrt(1.0f - u);
        sum += SampleEnvironment(dir);
    }
    return sum / kCount;
}

// 環境光による拡散反射
float3 AmbientDiffuse(float3 albedo, float3 N, float metallic)
{
    return EnvironmentDiffuse(N) * albedo * (1.0f - metallic) * gIndices.ambientIntensity;
}

// ノイズ除去済みのAOを取得する
float GetAO(uint2 pixel)
{
    if (gIndices.aoEnabled == 0)
    {
        return 1.0f;
    }
    float ao = GetAOOutput()[pixel.y * gIndices.width + pixel.x];
    return lerp(1.0f, ao, gIndices.aoStrength);
}

// 整数ハッシュ
uint PcgHash(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// ピクセル、フレーム、サンプル番号から[0,1)の乱数を2つ作る
float2 Random2(uint2 pixel, uint frame, uint sampleIndex)
{
    uint h = PcgHash(pixel.x + PcgHash(pixel.y + PcgHash(frame * 16u + sampleIndex)));
    uint h2 = PcgHash(h);
    return float2(h & 0xFFFFFF, h2 & 0xFFFFFF) / 16777216.0f;
}

// 三角形の面の法線(ワールド空間)
float3 FetchGeometricNormalWorld(BufferRef ref, uint primitiveIndex, float3x4 objectToWorld)
{
    uint start = primitiveIndex * 3;
    ByteAddressBuffer indexBuffer = GetBuffer(ref.indexHandle);
    ByteAddressBuffer vertexBuffer = GetBuffer(ref.vertexHandle);
    float3 p[3];
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        uint index = indexBuffer.Load<uint>((start + i + ref.indexOffset) * 4) + ref.vertexOffset;
        p[i] = mul(objectToWorld, float4(vertexBuffer.Load<float3>(index * VERTEX_STRIDE), 1.0f));
    }
    return normalize(cross(p[1] - p[0], p[2] - p[0]));
}

//=============================================================
// RAYRAY_TRACING_GAMES2のテクスチャのLODを実装
//=============================================================

// ワールド空間の三角形とUV
struct TriangleWorld
{
    float3 p0, p1, p2;
    float2 uv0, uv1, uv2;
};

TriangleWorld LoadTriangleWorld(BufferRef ref, uint primitiveIndex, float3x4 objectToWorld)
{
    uint start = primitiveIndex * 3;
    ByteAddressBuffer indexBuffer = GetBuffer(ref.indexHandle);
    ByteAddressBuffer vertexBuffer = GetBuffer(ref.vertexHandle);
    float3 p[3];
    float2 uv[3];
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        uint index = indexBuffer.Load<uint>((start + i + ref.indexOffset) * 4) + ref.vertexOffset;
        VertexData v = vertexBuffer.Load<VertexData>(index * VERTEX_STRIDE);
        p[i] = mul(objectToWorld, float4(v.position.xyz, 1.0f));
        uv[i] = v.texcoord;
    }
    TriangleWorld tri;
    tri.p0 = p[0]; tri.p1 = p[1]; tri.p2 = p[2];
    tri.uv0 = uv[0]; tri.uv1 = uv[1]; tri.uv2 = uv[2];
    return tri;
}

// テクスチャをサンプリングするためのLODの情報
struct TexLod
{
    bool useGrad;
    float2 ddx;       // 画面のx方向に1ピクセル進んだときのUVの変化
    float2 ddy;       // 画面のy方向に1ピクセル進んだときのUVの変化
    float coneLevel;  // テクスチャの大きさを除いたミップレベル
};

// 重心座標の、正規化デバイス座標に関する微分
float2x2 BarycentricDerivatives(float3 x, float3 n, float3 x0, float3 x1, float3 x2)
{
    float4x4 worldToTarget = transpose(gCamera.vpMatrix);
    float4x4 targetToWorld = transpose(mul(gCamera.mtxProjInv, gCamera.mtxViewInv));
    float4 xh = float4(x, 1.0f);

    // ワールド座標に関する重心座標の微分
    float3 A1 = x1 - x0;
    float3 A2 = x2 - x0;
    float3 Nt = cross(A1, A2);
    Nt /= dot(Nt, Nt);
    float3 du_dx = cross(A2, Nt);
    float3 dv_dx = cross(Nt, A1);

    // スクリーン座標に関するワールド座標の微分
    float3x3 dx_dxt = (float3x3) targetToWorld;
    dx_dxt[0] -= x.x * targetToWorld[3].xyz;
    dx_dxt[1] -= x.y * targetToWorld[3].xyz;
    dx_dxt[2] -= x.z * targetToWorld[3].xyz;
    float3 du_dxt = du_dx.x * dx_dxt[0] + du_dx.y * dx_dxt[1] + du_dx.z * dx_dxt[2];
    float3 dv_dxt = dv_dx.x * dx_dxt[0] + dv_dx.y * dx_dxt[1] + dv_dx.z * dx_dxt[2];

    // スクリーンのx, yに関する深度の微分
    float4 n4 = float4(n, -dot(n, x));
    n4 = mul(n4, targetToWorld);
    n4.z = max(abs(n4.z), 0.0001f) * (n4.z < 0.0f ? -1.0f : 1.0f);
    float2 ddepth_dXY = n4.xy / -n4.z;

    // まとめる
    float wMx = dot(worldToTarget[3], xh);
    float2 du_dXY = (du_dxt.xy + du_dxt.z * ddepth_dXY) * wMx;
    float2 dv_dXY = (dv_dxt.xy + dv_dxt.z * ddepth_dXY) * wMx;
    return float2x2(du_dXY, dv_dXY);
}

// 1次ヒットのLOD。UVの画面上の微分を求める
TexLod MakePrimaryTexLod(TriangleWorld tri, float3 hitPosition, float4x4 uvTransform)
{
    float3 n = normalize(cross(tri.p1 - tri.p0, tri.p2 - tri.p0));
    float2x2 duv = BarycentricDerivatives(hitPosition, n, tri.p0, tri.p1, tri.p2);

    // 重心座標の微分からUVの微分へ
    float2x2 dtc_duv = float2x2(tri.uv1.x - tri.uv0.x, tri.uv2.x - tri.uv0.x,
                                tri.uv1.y - tri.uv0.y, tri.uv2.y - tri.uv0.y);
    float2x2 dst = mul(dtc_duv, duv);

    // 正規化デバイス座標は画面の幅で2進むので、1ピクセルあたりに直す
    float2 ddxUV = float2(dst[0][0], dst[1][0]) * (2.0f / gIndices.width);
    float2 ddyUV = float2(dst[0][1], dst[1][1]) * (-2.0f / gIndices.height);

    TexLod lod;
    lod.useGrad = true;
    // UVのトランスフォームの分も微分に掛ける
    lod.ddx = mul(float4(ddxUV, 0.0f, 0.0f), uvTransform).xy;
    lod.ddy = mul(float4(ddyUV, 0.0f, 0.0f), uvTransform).xy;
    lod.coneLevel = 0.0f;
    return lod;
}

// 画面の1ピクセルが見込む角度
float PixelSpreadAngle()
{
    uint2 center = uint2(gIndices.width / 2, gIndices.height / 2);
    float3 d0 = GeneratePrimaryRay(center).Direction;
    float3 d1 = GeneratePrimaryRay(center + uint2(0, 1)).Direction;
    return length(d1 - d0);
}

// 2次以降のヒットのLOD。レイコーンの幅とUVの密度から、テクスチャの大きさを除いたミップレベルを求める
TexLod MakeConeTexLod(TriangleWorld tri, float4x4 uvTransform, float coneWidth, float3 rayDir)
{
    float2 uv0 = mul(float4(tri.uv0, 0.0f, 1.0f), uvTransform).xy;
    float2 uv1 = mul(float4(tri.uv1, 0.0f, 1.0f), uvTransform).xy;
    float2 uv2 = mul(float4(tri.uv2, 0.0f, 1.0f), uvTransform).xy;
    float2 e1 = uv1 - uv0;
    float2 e2 = uv2 - uv0;
    float uvArea = abs(e1.x * e2.y - e1.y * e2.x);
    float3 worldCross = cross(tri.p1 - tri.p0, tri.p2 - tri.p0);
    float worldArea = length(worldCross);

    // 斜めから見るほど、面の上での円錐の幅は広がる
    float cosTheta = abs(dot(worldCross / max(worldArea, 1e-12f), rayDir));

    TexLod lod;
    lod.useGrad = false;
    lod.ddx = float2(0, 0);
    lod.ddy = float2(0, 0);
    lod.coneLevel = 0.5f * log2(max(uvArea, 1e-12f) / max(worldArea, 1e-12f))
                  + log2(max(coneWidth, 1e-6f) / max(cosTheta, 0.05f));
    return lod;
}

// LODを考慮してテクスチャをサンプリングする
float4 SampleTexture(uint handle, float2 uv, TexLod lod)
{
    Texture2D tex = GetTexture(handle);
    if (gIndices.texLodEnabled == 0)
    {
        return tex.SampleLevel(gSampler, uv, 0);
    }
    if (lod.useGrad)
    {
        // バイアスは微分の大きさで表す
        float scale = exp2(gIndices.texLodBias);
        return tex.SampleGrad(gSampler, uv, lod.ddx * scale, lod.ddy * scale);
    }
    uint width, height, levels;
    tex.GetDimensions(0, width, height, levels);
    float level = lod.coneLevel + 0.5f * log2(float(width) * float(height)) + gIndices.texLodBias;
    return tex.SampleLevel(gSampler, uv, clamp(level, 0.0f, float(levels - 1)));
}

// IBLを反射色に依存しない部分と反射色に掛かる重みに分ける
void SplitIBL(float3 albedo, float3 N, float3 V, float metallic, float roughness, out float3 ambient, out float3 reflectWeight)
{
    ambient = CalculateIBL(albedo, float3(0, 0, 0), N, V, metallic, roughness);
    reflectWeight = CalculateIBL(albedo, float3(1, 1, 1), N, V, metallic, roughness) - ambient;
}

inline float MaxComponent(float3 v) { return max(v.x, max(v.y, v.z)); }

// ============================================================
// 2次以降のヒット用の軽量シェーディング
// ============================================================

struct LiteResult
{
    float3 emitted;    // このヒットで確定した色
    float3 weight;     // 次のレイの結果に掛ける重み
    float3 nextDir;    // 次のレイの方向
    uint nextFlags;    // 次のレイのフラグ
};

LiteResult ShadeLiteDefault(SurfaceHit s, RQHit hit, float3 rayDir, float coneWidth)
{
    LiteResult r;
    r.emitted = float3(0, 0, 0);
    r.weight = float3(0, 0, 0);
    r.nextDir = rayDir;
    r.nextFlags = RAY_FLAG_NONE;

    MaterialData material = GetBuffer(s.ref.MaterialIndex).Load<MaterialData>(0);
    float2 uv = mul(float4(s.vtx.texcoord, 0.0f, 1.0f), material.uvTransform).xy;
    TexLod lod = MakeConeTexLod(LoadTriangleWorld(s.ref, hit.primitiveIndex, s.objectToWorld), material.uvTransform, coneWidth, rayDir);

    float3 localNormal = s.vtx.normal;
    if (material.normalTextureHandle != 0)
    {
        float4 normalMapColor = SampleTexture(material.normalTextureHandle, uv, lod);
        localNormal = GetNormalFromMap(normalMapColor, s.vtx.normal, float4(normalize(s.vtx.tangent.xyz), s.vtx.tangent.w));
    }
    float3 N = ObjectNormalToWorld(localNormal, s.objectToWorld);
    float3 V = normalize(gCamera.worldPosition.xyz - s.worldPosition);
    if (dot(N, V) < 0.0f) { N = -N; }

    float3 albedo = material.color.rgb * SampleTexture(material.textureHandle, uv, lod).rgb;
    if (!material.enableLighting)
    {
        r.emitted = albedo;
        return r;
    }

    float3 L = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
    float3 direct = CalculateBRDF(albedo, N, V, L, lightColor, material.roughness, material.metallic);

    float3 ambient, reflectWeight;
    SplitIBL(albedo, N, V, material.metallic, material.roughness, ambient, reflectWeight);
    float3 reflectDir = reflect(rayDir, N);

    // 背景からの環境光
    ambient += AmbientDiffuse(albedo, N, material.metallic);

    if (s.ref.type != 1)
    {
        // 不透明は反射方向へ続ける
        r.emitted = direct + ambient;
        r.weight = reflectWeight;
        r.nextDir = reflectDir;
        return r;
    }

    // 半透明
    float alpha = material.color.a;
    float3 surface = direct + ambient;
    // 表面なら空気中->屈折媒質、裏面なら屈折媒質->空気中
    float3 refracted = (dot(N, rayDir) < 0.0f)
        ? refract(rayDir, N, 1.0f / material.ior)
        : refract(rayDir, -N, material.ior);
    if (length(refracted) < 0.01f)
    {
        // 全反射は屈折も反射色になる
        r.emitted = alpha * surface;
        r.weight = alpha * reflectWeight + (1.0f - alpha);
        r.nextDir = reflectDir;
        return r;
    }

    // 寄与の大きい方だけレイを飛ばし、もう片方は背景で代用する
    if ((1.0f - alpha) >= alpha * MaxComponent(reflectWeight))
    {
        r.emitted = alpha * (surface + reflectWeight * SampleEnvironment(reflectDir));
        r.weight = (1.0f - alpha).xxx;
        r.nextDir = refracted;
        r.nextFlags = RAY_FLAG_CULL_BACK_FACING_TRIANGLES;
    }
    else
    {
        r.emitted = alpha * surface + (1.0f - alpha) * SampleEnvironment(refracted);
        r.weight = alpha * reflectWeight;
        r.nextDir = reflectDir;
    }
    return r;
}

LiteResult ShadeLiteIce(SurfaceHit s, RQHit hit, float3 rayDir, float coneWidth)
{
    float hitT = hit.t;
    LiteResult r;
    r.emitted = float3(0, 0, 0);
    r.weight = float3(0, 0, 0);
    r.nextDir = rayDir;
    r.nextFlags = RAY_FLAG_NONE;

    IceMaterialData material = GetBuffer(s.ref.MaterialIndex).Load<IceMaterialData>(0);
    float2 uv = mul(float4(s.vtx.texcoord, 0.0f, 1.0f), material.uvTransform).xy;
    TexLod lod = MakeConeTexLod(LoadTriangleWorld(s.ref, hit.primitiveIndex, s.objectToWorld), material.uvTransform, coneWidth, rayDir);

    float3 localNormal = s.vtx.normal;
    if (material.normalTextureHandle != 0)
    {
        float4 normalMapColor = SampleTexture(material.normalTextureHandle, uv, lod);
        localNormal = GetNormalFromMap(normalMapColor, s.vtx.normal, float4(normalize(s.vtx.tangent.xyz), s.vtx.tangent.w));
    }
    float3 worldNormal = normalize(mul((float3x3) s.objectToWorld, localNormal));
    float3 V = normalize(gCamera.worldPosition.xyz - s.worldPosition);
    if (dot(worldNormal, V) < 0.0f) { worldNormal = -worldNormal; }

    // 裏面確認
    bool entering = dot(rayDir, worldNormal) < 0.0f;
    float3 N = entering ? worldNormal : -worldNormal;
    float eta = entering ? (1.0f / material.ior) : material.ior;

    float3 iceColor = material.color.rgb * SampleTexture(material.textureHandle, uv, lod).rgb;

    // Fresnel反射率
    float cosTheta = saturate(dot(-rayDir, N));
    float r0 = (1.0f - material.ior) / (1.0f + material.ior);
    r0 = max(r0 * r0, 0.3f);
    float F = saturate(r0 + (1.0f - r0) * pow(1.0f - cosTheta, 5.0f));

    // Beer-Lambert
    float3 transmittance = float3(1.0f, 1.0f, 1.0f);
    if (!entering)
    {
        float absorpScale = 1.0f - material.roughness * 0.6f;
        transmittance = BeerLambert(ICE_ABSORPTION_COEFF * absorpScale, hitT);
    }

    float3 L = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
    float3 reflectDir = reflect(rayDir, N);
    float3 refracted = refract(rayDir, N, eta);

    if (length(refracted) < 0.001f)
    {
        // 全反射
        r.weight = transmittance;
        r.nextDir = reflectDir;
    }
    else if (F <= 0.5f)
    {
        r.emitted = F * SampleEnvironment(reflectDir);
        r.weight = (1.0f - F) * transmittance * iceColor;
        r.nextDir = refracted;
    }
    else
    {
        r.emitted = (1.0f - F) * SampleEnvironment(refracted) * transmittance * iceColor;
        r.weight = F.xxx;
        r.nextDir = reflectDir;
    }

    // 近似SSS
    if (entering && material.roughness > 0.01f)
    {
        r.emitted += IceFakeSSS(N, L, rayDir, lightColor, material.roughness * 0.4f) * (1.0f - F);
    }
    // 鏡面ハイライト
    if (gDirectionalLight.active)
    {
        r.emitted += CalcSpecular(worldNormal, L, V, lightColor, material.specularColor.rgb, material.shininess);
    }
    // リムライト
    float rimFactor = pow(1.0f - saturate(dot(worldNormal, V)), material.rimPower);
    r.emitted += material.rimColor.rgb * rimFactor * material.rimIntensity * gDirectionalLight.intensity;
    return r;
}

// 反射・屈折レイの色を求める。
float3 TraceRadiance(float3 origin, float3 direction, uint rayFlags, int recursive, float coneWidth)
{
    float3 radiance = float3(0, 0, 0);
    float3 throughput = float3(1, 1, 1);
    // 面での反射・屈折による広がりの変化は無視し、画面の1ピクセル分の広がりのまま進める
    float spreadAngle = PixelSpreadAngle();

    [loop]
    for (int bounce = recursive + 1; ; ++bounce)
    {
        RQHit hit;
        uint material;
        if (!TraceClosest(MakeRay(origin, direction), rayFlags, RAY_MASK_ALL, hit, material) || bounce >= RQ_MAX_RECURSIVE)
        {
            // ミス、または再帰の上限に達したら背景
            radiance += throughput * SampleEnvironment(direction);
            break;
        }

        SurfaceHit s = LoadSurfaceHit(hit);
        // 進んだ距離の分だけ円錐が広がる
        coneWidth += spreadAngle * hit.t;
        LiteResult r;
        if (material == RQ_MATERIAL_ICE)
        {
            r = ShadeLiteIce(s, hit, direction, coneWidth);
        }
        else
        {
            r = ShadeLiteDefault(s, hit, direction, coneWidth);
        }

        radiance += throughput * r.emitted;
        throughput *= r.weight;
        if (MaxComponent(throughput) < 0.001f)
        {
            break;
        }

        origin = s.worldPosition;
        direction = r.nextDir;
        rayFlags = r.nextFlags;
    }
    return radiance;
}

// ============================================================
// シェーディングパスの共通処理
// ============================================================

// このスレッドが担当するピクセルとヒット情報を取得する
bool FetchShadePixel(uint threadIndex, uint materialType, out uint2 pixel, out RQHit hit)
{
    pixel = uint2(0, 0);
    hit = (RQHit) 0;
    if (threadIndex >= GetCounter()[materialType])
    {
        return false;
    }
    uint packed = GetPixelList()[materialType * GetPixelCount() + threadIndex];
    pixel = uint2(packed & 0xFFFF, packed >> 16);
    hit = GetHitBuffer()[pixel.y * gIndices.width + pixel.x];
    return true;
}

#endif
