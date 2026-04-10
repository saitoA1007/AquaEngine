#include"ParamEditorManager.h"
#include<cassert>

#include"ImguiManager.h"

using namespace GameEngine;

ParamEditorManager* ParamEditorManager::GetInstance() {
	static ParamEditorManager instance;
	return &instance;
}
