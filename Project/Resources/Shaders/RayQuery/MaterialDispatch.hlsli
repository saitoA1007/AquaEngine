// 自動生成ファイル。RayQueryMaterialRegistryが書き出すため直接編集しない
#ifndef MATERIAL_DISPATCH_HLSLI
#define MATERIAL_DISPATCH_HLSLI
#include "RayQueryCommon.hlsli"
#include "Materials/DefaultMaterial.hlsli"
#include "Materials/IceMaterial.hlsli"

// マテリアルIDから対応するシェーディング関数を呼び出す
ShadeResult EvaluateMaterial(HitInfo hit, float3 rayDir)
{
    switch (hit.materialId)
    {
        case MATERIAL_ID_ICE:
            return ShadeIceMaterial(hit, rayDir);

        case MATERIAL_ID_DEFAULT:
        default:
            return ShadeDefaultMaterial(hit, rayDir);
    }
}

#endif
