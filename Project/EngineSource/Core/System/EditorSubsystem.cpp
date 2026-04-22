#ifdef USE_IMGUI
#include "EditorSubsystem.h"
#include "ResourceSubsystem.h"
#include "GraphicsSubsystem.h"
#include "SceneSubsystem.h"
#include "InputSubsystem.h"
#include "ResourceSubsystem.h"
using namespace GameEngine;

void EditorSubsystem::Initialize() {
    auto* graphics = context_.graphics;
    auto* resource = context_.resource;
    auto* scene = context_.scene;

    auto gridModel = resource->GetModelManager()->GetNameByModel("Grid");

    editorCore_ = std::make_unique<EditorCore>();
    editorCore_->Initialize(
        resource->GetTextureManager(),
        scene->GetSceneChangeRequest(),
        graphics->GetRenderPassCtrl(),
        context_.input->GetInput(),
        graphics->GetRenderQueue(),
        graphics->GetDebugRenderer(),
        gridModel,
        resource->GetGameParamEditor());
}

void EditorSubsystem::Update() {
    editorCore_->Run();
}

void EditorSubsystem::Finalize() {
    editorCore_->Finalize();
}

bool EditorSubsystem::IsActiveUpdate() const {
    return editorCore_->IsActiveUpdate();
}

bool EditorSubsystem::IsPause() const {
    return editorCore_->IsPause();
}
#endif