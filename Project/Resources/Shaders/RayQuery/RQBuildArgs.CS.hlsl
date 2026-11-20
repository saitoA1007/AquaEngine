#include "RQCommon.hlsli"

// マテリアルごとのピクセル数からDispatch引数を作る
[numthreads(RQ_MATERIAL_COUNT, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint count = GetCounter()[id.x];
    GetDispatchArgs()[id.x] = uint3((count + RQ_SHADE_GROUP_SIZE - 1) / RQ_SHADE_GROUP_SIZE, 1, 1);

    // 次のフレームのRTAOの再投影用に、今フレームのカメラを保存する
    if (id.x == 0)
    {
        RQPrevCamera prev;
        prev.vpMatrix = gCamera.vpMatrix;
        prev.worldPosition = gCamera.worldPosition;
        // AOを使っていないフレームの後は履歴を使わない
        prev.valid = gIndices.aoEnabled;
        GetPrevCamera()[0] = prev;
    }
}
