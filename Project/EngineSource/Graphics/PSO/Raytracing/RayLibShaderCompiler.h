#pragma once
#include <vector>
#include "DXC.h"

namespace GameEngine {

	class RayLibShaderCompiler {
	public:
		struct LibraryResult {
			Microsoft::WRL::ComPtr<IDxcBlob> blob;
			std::vector<std::wstring> exportNames;
		};

	public:

		void Initialize(DXC* dxc);

		// シェーダーをコンパイルする
		LibraryResult CompileShader(const std::wstring& hlslPath);

	private:
		DXC* dxc_ = nullptr;

		const std::wstring csoDirectory_ = L"Resources/Shaders/Compiled/";
		const std::wstring kLibProfile_ = L"lib_6_5";

	private:

		// コンパイル済みBlobからエクスポート名を取得
		std::vector<std::wstring> ReflectExportNames(IDxcBlob* blob);
	};
}