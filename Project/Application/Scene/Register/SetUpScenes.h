#pragma once
#include "SceneRegistry.h"

// 各シーン
#include "Application/Scene/TestScene.h"
#include "Application/Scene/GameScene.h"
#include "Application/Scene/IceDemoScene.h"
#include "Application/Scene/AODemoScene.h"

using namespace GameEngine;

/// <summary>
/// シーンを登録する
/// </summary>
/// <param name="factory"></param>
void SetupScenes(SceneRegistry& factory) {

    // 各シーンの登録
    factory.RegisterScene<TestScene>("Test");
    factory.RegisterScene<GameScene>("Game");
#ifdef USE_IMGUI
    factory.RegisterScene<AODemoScene>("AODemo");
    factory.RegisterScene<IceDemoScene>("IceDemo");
#endif
   

    // 立ち上げ時に起動するシーン
    factory.SetDefaultScene("Game");
}