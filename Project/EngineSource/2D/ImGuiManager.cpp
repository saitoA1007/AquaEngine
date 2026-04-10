#include"ImGuiManager.h"
using namespace GameEngine;

void ImGuiManager::Initialize([[maybe_unused]]ID3D12Device* device, [[maybe_unused]] ID3D12GraphicsCommandList* commandList, [[maybe_unused]] DXGI_SWAP_CHAIN_DESC1 swapChainDesc,
	[[maybe_unused]] WindowsApp* windowsApp, [[maybe_unused]] SrvManager* srvManager) {
#ifdef USE_IMGUI
	commandList_ = commandList;
	windowsApp_ = windowsApp;
	srvManager_ = srvManager;

	// ImGuiの初期化。
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.Fonts->Build();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_ = {};
	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	uint32_t index = srvManager_->AllocateSrvIndex(SrvHeapType::Other);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvManager_->GetCPUHandle(index);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvManager_->GetGPUHandle(index);

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(windowsApp_->GetHwnd());
	ImGui_ImplDX12_Init(device,
		swapChainDesc.BufferCount,
		rtvDesc_.Format,
		srvManager_->GetSRVHeap(),
		cpuHandle,
		gpuHandle);

	ed::Config config;
	config.SettingsFile = "node_editor_docked.json";
	g_NodeContext = ed::CreateEditor(&config);
#endif
}

void ImGuiManager::BeginFrame() {
#ifdef USE_IMGUI
	// ImGuiにフレームが始まる旨を伝える
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	//ImGui::Begin("NodeEditor");
	//ImGui::Separator();
	//ed::SetCurrentEditor(g_NodeContext);
	//ed::Begin("My Editor", ImVec2(0.0, 0.0f));
	//int uniqueId = 1;
	//// Start drawing nodes.
	//ed::BeginNode(uniqueId++);
	//ImGui::Text("Node A");
	//ed::BeginPin(uniqueId++, ed::PinKind::Input);
	//ImGui::Text("-> In");
	//ed::EndPin();
	//ImGui::SameLine();
	//ed::BeginPin(uniqueId++, ed::PinKind::Output);
	//ImGui::Text("Out ->");
	//ed::EndPin();
	//ed::EndNode();
	//ed::End();
	//ed::SetCurrentEditor(nullptr);
	//ImGui::End();
#endif
}

void ImGuiManager::EndFrame() {
#ifdef USE_IMGUI
	// ImGuiの内部コマンドを生成する
	ImGui::Render();
#endif
}

void ImGuiManager::Draw() {
#ifdef USE_IMGUI
	// 実際のcommandListのImGuiの描画コマンドを積む
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_);
#endif
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}