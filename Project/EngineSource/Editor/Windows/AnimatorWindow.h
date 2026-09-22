#pragma once
#include "IEditorWindow.h"

namespace GameEngine {

	class AnimatorWindow : public IEditorWindow {
	public:
		AnimatorWindow();

		void Draw() override;
		std::string GetName() const override { return "Animator"; };

	private:


	};
}
