#pragma once
#include <dxgi1_6.h>

#include"WindowsApp.h"
#include"SrvManager.h"

#ifdef USE_IMGUI
#include"Externals/imgui/imgui.h"
#include"Externals/imgui/imgui_impl_dx12.h"
#include"Externals/imgui/imgui_impl_win32.h"
#include"Externals/imgui/ImGuizmo.h"

#include "Externals/imgui/imgui_node_editor.h"
namespace ed = ax::NodeEditor;

static ed::EditorContext* g_NodeContext = nullptr;
#endif

namespace GameEngine {

	class ImGuiManager final {
	public:
		ImGuiManager() = default;
		~ImGuiManager() = default;

		/// <summary>
		/// 初期処理
		/// </summary>
		/// <param name="windowsApp"></param>
		/// <param name="dxCommon"></param>
		void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXGI_SWAP_CHAIN_DESC1 swapChainDesc,
			WindowsApp* windowsApp,SrvManager* srvManager);

		/// <summary>
		/// 更新前処理
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// 更新後処理
		/// </summary>
		void EndFrame();

		/// <summary>
		/// 描画処理
		/// </summary>
		void Draw();

		/// <summary>
		/// 終了処理
		/// </summary>
		void Finalize();

	private:
		ImGuiManager(const ImGuiManager&) = delete;
		const ImGuiManager& operator=(const ImGuiManager&) = delete;

		ID3D12GraphicsCommandList* commandList_ = nullptr;
		WindowsApp* windowsApp_ = nullptr;
		SrvManager* srvManager_ = nullptr;
	};
}
