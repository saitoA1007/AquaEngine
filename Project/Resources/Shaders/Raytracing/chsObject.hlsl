#include "Common.hlsli"

struct VertexData {
    float3 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

StructuredBuffer<uint> indexBuffer : register(t0, space1);
StructuredBuffer<VertexData> vertexBuffer : register(t1, space1);

VertexData GetHitVertex(MyAttribute attrib) {
    VertexData v = (VertexData) 0;
    float3 barycentrics = CalcBarycentrics(attrib.barys);
    uint start = PrimitiveIndex() * 3; // Triangle List のため.

    float3 positions[3], normals[3];
    float2 texcoords[3];
    for (int i = 0; i < 3; ++i)
    {
        uint index = indexBuffer[start + i];
        positions[i] = vertexBuffer[index].position;
        normals[i] = vertexBuffer[index].normal;
        texcoords[i] = vertexBuffer[index].texcoord;
    }
    v.position = CalcHitAttribute3(positions, attrib.barys);
    v.texcoord = CalcHitAttribute2(texcoords, attrib.barys);
    v.normal = CalcHitAttribute3(normals, attrib.barys);
    v.normal = normalize(v.normal);
    return v;
}

[shader("closesthit")]
void MainObjectCHS(inout Payload payload, MyAttribute attrib) {
    if (checkRecursiveLimit(payload))
    {
        return;
    }
    
    
    VertexData vtx = GetHitVertex(attrib);
    
    
    float3 worldPosition = mul(float4(vtx.position, 1), ObjectToWorld4x3());
    float3 worldNormal = mul(vtx.normal, (float3x3) ObjectToWorld4x3());
    
    // 平行光源でライティング.
    float3 lightDir = -normalize(gSceneParam.lightDirection.xyz);
    float dotNL = saturate(dot(worldNormal, lightDir));
    payload.color = dotNL * sphereDiffuse;

    payload.color += gSceneParam.ambientColor.xyz * sphereDiffuse;
}