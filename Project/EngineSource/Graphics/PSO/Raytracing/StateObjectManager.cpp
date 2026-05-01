#include "StateObjectManager.h"

using namespace GameEngine;

void StateObjectManager::Initialize(DXC* dxc) {
	// シェーダコンパイル機能を初期化
	rayLibShaderCompiler_.Initialize(dxc);


}