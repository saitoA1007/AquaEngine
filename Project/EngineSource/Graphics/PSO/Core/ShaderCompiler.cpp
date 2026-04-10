#include"ShaderCompiler.h"
#include <cassert>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;
using namespace GameEngine;

void ShaderCompiler::Initialize(DXC* dxc) {
	dxc_ = dxc;

	// CSOディレクトリが存在しない場合は作成
	fs::path csoDir(csoDirectory_);
	if (!fs::exists(csoDir)) {
		fs::create_directories(csoDir);
	}
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::CompileShader(Type type, const std::wstring& path) {
	std::wstring csoPath = GetCsoPath(path);

#ifdef _DEBUG
	// Debug版ではHLSLが更新されていればコンパイル、なければCSOを読み込む
	if (!fs::exists(csoPath) || IsHlslNewer(path, csoPath)) {
		// コンパイルして保存
		return CompileAndSave(type, path);
	} else {
		// 既存のCSOを読み込み
		return LoadCsoFile(csoPath);
	}
#else
	// Release版ではCSOファイルを読み込む
	Microsoft::WRL::ComPtr<IDxcBlob> blob = LoadCsoFile(csoPath);
	assert(blob != nullptr && "CSO file not found in Release build");
	return blob;
#endif
}

std::wstring ShaderCompiler::GetCsoPath(const std::wstring& hlslPath) {
	// パスからファイル名を取得
	fs::path path(hlslPath);
	std::wstring filename = path.filename().wstring();

	// 拡張子を.csoに変更
	size_t dotPos = filename.find_last_of(L'.');
	if (dotPos != std::wstring::npos) {
		filename = filename.substr(0, dotPos) + L".cso";
	} else {
		filename += L".cso";
	}

	// CSOディレクトリのパスと結合
	return csoDirectory_ + filename;
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::LoadCsoFile(const std::wstring& csoPath) {
	// ファイルを開く
	std::ifstream file(csoPath, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		return nullptr;
	}

	// ファイルサイズを取得
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	// データを読み込む
	std::vector<char> buffer(size);
	if (!file.read(buffer.data(), size)) {
		return nullptr;
	}
	file.close();

	// IDxcBlobを作成
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob;
	HRESULT hr = dxc_->dxcUtils_->CreateBlob(
		buffer.data(),
		static_cast<UINT32>(size),
		CP_ACP,
		&blob
	);

	if (FAILED(hr)) {
		return nullptr;
	}

	return blob;
}

void ShaderCompiler::SaveCsoFile(const std::wstring& csoPath, IDxcBlob* blob) {
	// ファイルに書き込む
	std::ofstream file(csoPath, std::ios::binary);
	if (!file.is_open()) {
		assert(false && "Failed to open CSO file for writing");
		return;
	}
	file.write(static_cast<const char*>(blob->GetBufferPointer()),blob->GetBufferSize());
	file.close();
}

bool ShaderCompiler::IsHlslNewer(const std::wstring& hlslPath, const std::wstring& csoPath) {
	fs::path hlsl(hlslPath);
	fs::path cso(csoPath);

	if (!fs::exists(hlsl) || !fs::exists(cso)) {
		return true;
	}

	// 更新日時を比較
	auto hlslTime = fs::last_write_time(hlsl);
	auto csoTime = fs::last_write_time(cso);

	return hlslTime > csoTime;
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::CompileAndSave(Type type, const std::wstring& hlslPath) {
	// HLSLをコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = dxc_->CompileShader(
		hlslPath,
		compileTypes[static_cast<size_t>(type)].c_str(),
		dxc_->dxcUtils_.Get(),
		dxc_->dxcCompiler_.Get(),
		dxc_->includeHandler_.Get()
	);

	assert(shaderBlob != nullptr && "Shader compilation failed");

	// CSOファイルとして保存
	std::wstring csoPath = GetCsoPath(hlslPath);
	SaveCsoFile(csoPath, shaderBlob.Get());
	return shaderBlob;
}