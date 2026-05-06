#include "Common.hlsli"

[shader("miss")]
void MainMiss(inout Payload payload)
{
    payload.color.xyz = 0.1;
}
