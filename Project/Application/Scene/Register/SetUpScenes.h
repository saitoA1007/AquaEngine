#pragma once
#include"SceneRegistry.h"

// 各シーン
#include "Application/Scene/TitleScene.h"
#include "Application/Scene/GameScene.h"
#include "Application/Scene/GEScene.h"

using namespace GameEngine;

/// <summary>
/// シーンを登録する
/// </summary>
/// <param name="factory"></param>
void SetupScenes(SceneRegistry& factory) {

    // 各シーンの登録
    factory.RegisterScene<TitleScene>("Title");
    factory.RegisterScene<GameScene>("Game");
    factory.RegisterScene<GEScene>("GE");

    // 立ち上げ時に起動するシーン
    factory.SetDefaultScene("Title");
}