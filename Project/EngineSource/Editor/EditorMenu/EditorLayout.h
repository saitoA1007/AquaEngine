#pragma once
#include <string>
#include <vector>

#include "Windows/IEditorWindow.h"

#include<json.hpp>
using json = nlohmann::json;

namespace GameEngine {

	class EditorLayout {
	public:

		/// <summary>
		/// ウィンドウレイアウトをjsonファイルに保存する
		/// </summary>
		/// <param name="windows"></param>
		void SaveLayout(const std::vector<std::unique_ptr<IEditorWindow>>& windows) const;

		/// <summary>
		/// jsonファイルからウィンドウレイアウトを取得する
		/// </summary>
		/// <param name="windows"></param>
		void LoadLayout(std::vector<std::unique_ptr<IEditorWindow>>& windows);

	private:

		// ウィンドウレイアウトが保存してあるファイルパス名
		const std::string& kFilePath = "Resources/Json/DebugData/windowLayout.json";
	};
}
