// パストレーシングの最初のヒットを求めるためのビジビリティバッファ
// レイトレーシングと同じ頂点データ(gBufferData)を読んでラスタライズし、
// 画素ごとに「TLASのインスタンス番号+1」と「三角形の番号」を書き込む(0なら何も無い)
// 交点の重心座標や距離は、パストレーシング側でカメラレイと三角形の交差から求め直す

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
    float4x4 mtxViewInv;
    float4x4 mtxProjInv;
};

struct BufferRef
{
    uint type;
    uint MaterialIndex;
    uint indexHandle;
    uint vertexHandle;
    uint vertexOffset;
    uint indexOffset;
    float2 pad;
};

// 描画するインスタンスの情報
cbuffer InstanceConstants : register(b0)
{
    uint gInstanceIndex; // TLASのインスタンス番号
    uint gRefIndex; // BufferRefの番号(InstanceID)
    uint gHitGroupIndex; // ヒットグループ。氷(1)はインデックス・頂点のオフセットを使う
    uint gPadding;
    float4 gTransformRow0; // ワールド変換行列(3x4)の各行
    float4 gTransformRow1;
    float4 gTransformRow2;
};
ConstantBuffer<Camera> gCamera : register(b1);
StructuredBuffer<BufferRef> gBufferRefs : register(t0, space2);
ByteAddressBuffer gBufferData[] : register(t0, space3);

// 頂点データのストライド。位置は先頭にある
static const uint VERTEX_STRIDE = 52;
static const uint HIT_GROUP_ICE = 1;

struct VSOutput
{
    float4 position : SV_Position;
};

VSOutput MainVisibilityVS(uint vertexID : SV_VertexID)
{
    // RayQuery.CS.hlslのLoadHitVertexと同じ規則で頂点を読む
    BufferRef ref = gBufferRefs[gRefIndex];
    bool applyOffset = gHitGroupIndex == HIT_GROUP_ICE;
    uint indexOffset = applyOffset ? ref.indexOffset : 0;
    uint vertexOffset = applyOffset ? ref.vertexOffset : 0;

    uint localIndex = gBufferData[ref.indexHandle].Load<uint>(vertexID * 4 + indexOffset * 4);
    float3 localPosition = gBufferData[ref.vertexHandle].Load<float3>((localIndex + vertexOffset) * VERTEX_STRIDE);

    float4 p = float4(localPosition, 1.0f);
    float3 worldPosition = float3(dot(gTransformRow0, p), dot(gTransformRow1, p), dot(gTransformRow2, p));

    VSOutput output;
    output.position = mul(float4(worldPosition, 1.0f), gCamera.vpMatrix);
    return output;
}

uint2 MainVisibilityPS(VSOutput input, uint primitiveID : SV_PrimitiveID) : SV_Target0
{
    return uint2(gInstanceIndex + 1, primitiveID);
}
