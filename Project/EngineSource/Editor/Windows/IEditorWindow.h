#pragma once
#include<string>

namespace GameEngine {

	class IEditorWindow {
	public:
		virtual ~IEditorWindow() = default;
		virtual void Draw() = 0;
		virtual std::string GetName() const = 0;

		bool isActive = true;
	};
}
