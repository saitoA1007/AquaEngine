#include"Engine.h"
#include "ResourceLeakChecker.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	// リソースチェックのデバック
	D3DResourceLeakChecker leakCheck;

	// エンジンの実行を開始
	std::unique_ptr<GameEngine::Engine> engine = std::make_unique<GameEngine::Engine>();
	engine->RunEngine(hInstance);
	return 0;
}