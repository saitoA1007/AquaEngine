struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
};

struct Player
{
    float4 position;
};

StructuredBuffer<uint> indexBuffer : register(t0);
StructuredBuffer<VertexData> vertexBuffer : register(t1);
RWStructuredBuffer<Player> gPlayer : register(u0);

cbuffer Constants : register(b0)
{
    uint triangleCount; // indexBuffer.Length / 3
};

// Möller–Trumboreアルゴリズムによるレイ-三角形交差判定
bool RayTriangleIntersect(
    float3 rayOrigin, float3 rayDir,
    float3 v0, float3 v1, float3 v2,
    out float t, out float3 hitNormal)
{
    t = -1.0f;
    hitNormal = float3(0, 1, 0);

    float3 edge1 = v1 - v0;
    float3 edge2 = v2 - v0;
    float3 h = cross(rayDir, edge2);
    float a = dot(edge1, h);

    // 平行チェック（裏面も検出したい場合は abs(a) < EPSILON）
    if (a < 1e-6f)
        return false;

    float f = 1.0f / a;
    float3 s = rayOrigin - v0;
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    float3 q = cross(s, edge1);
    float v = f * dot(rayDir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * dot(edge2, q);
    if (t < 0.0f)
        return false; // 後方は無視

    hitNormal = normalize(cross(edge1, edge2));
    return true;
}

[numthreads(1, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    float3 playerPos = gPlayer[id.x].position.xyz;

    // 真下にレイを飛ばす
    float3 rayOrigin = playerPos + float3(0, 100.0f, 0); // 上から余裕を持って
    float3 rayDir = float3(0, -1, 0);

    float closestT = 1e9f;
    float3 closestNormal = float3(0, 1, 0);
    bool hit = false;

    for (uint i = 0; i < triangleCount; i++)
    {
        uint i0 = indexBuffer[i * 3 + 0];
        uint i1 = indexBuffer[i * 3 + 1];
        uint i2 = indexBuffer[i * 3 + 2];

        float3 v0 = vertexBuffer[i0].position.xyz;
        float3 v1 = vertexBuffer[i1].position.xyz;
        float3 v2 = vertexBuffer[i2].position.xyz;

        float t;
        float3 n;
        if (RayTriangleIntersect(rayOrigin, rayDir, v0, v1, v2, t, n))
        {
            if (t < closestT)
            {
                closestT = t;
                closestNormal = n;
                hit = true;
            }
        }
    }

    if (hit)
    {
        float3 hitPos = rayOrigin + rayDir * closestT;

        // プレイヤーをMesh表面に吸着（足元オフセット考慮）
        float playerHeight = 1.0f;
        gPlayer[id.x].position.xyz = hitPos + float3(0, playerHeight, 0);

        // 法線も保存したい場合はPlayer構造体に追加
        // gPlayer[id.x].groundNormal = closestNormal;
    }
}