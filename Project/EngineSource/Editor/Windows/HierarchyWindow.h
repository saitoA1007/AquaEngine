#pragma once
#include "IEditorWindow.h"
#include "ImGuiManager.h"

namespace GameEngine {

    class HierarchyWindow : public IEditorWindow {
    public:

        void Draw() override;
        std::string GetName() const override { return "ParameterHierarchy"; }
    };
}
