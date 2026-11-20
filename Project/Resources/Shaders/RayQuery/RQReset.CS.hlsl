#include "RQCommon.hlsli"

// マテリアルごとのピクセル数を0にする
[numthreads(RQ_MATERIAL_COUNT, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    GetCounter()[id.x] = 0;
}
