#include "AddObjectBar.h"
#include <filesystem>
#include "ImGuiManager.h"
#include "MyMath.h"
#include "DebugCamera.h"
using namespace GameEngine;

AddObjectBar::AddObjectBar(StaticGameObjectManager* staticObjectManager, RenderQueue* renderQueue, DebugCamera* debugCamera) {
	staticObjectManager_ = staticObjectManager;
	renderQueue_ = renderQueue;
    debugCamera_ = debugCamera;
}

void AddObjectBar::Run() {

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Object")) {
			if (ImGui::MenuItem("Cube")) {
				staticObjectManager_->AddObject("CubeObject", "Cube");
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}	
}

void AddObjectBar::ApplyGuizmo() {

    ImGuiIO& io = ImGui::GetIO();

    // マウスがクリックされた時
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) {

        // 直前に描画したImGui::Imageの左上座標
        ImVec2 imageMin = ImGui::GetItemRectMin(); 
        ImVec2 imageSize = ImGui::GetItemRectSize();
        // スクリーン座標のマウス位置を取得
        Vector2 mousePosGlobal = Vector2(io.MousePos.x, io.MousePos.y);

        float relX = mousePosGlobal.x - imageMin.x;
        float relY = mousePosGlobal.y - imageMin.y;

        // マウスがゲーム画面の範囲内にいるときだけ処理
        if (relX >= 0.0f && relX <= imageSize.x &&
            relY >= 0.0f && relY <= imageSize.y) {

            Vector2 mousePosInGame;
            mousePosInGame.x = (relX / imageSize.x) * 1280.0f;
            mousePosInGame.y = (relY / imageSize.y) * 720.0f;

            int32_t id = -1;

            if (renderQueue_->GetUseDebugCamera()) {
                id = staticObjectManager_->SelectObject(mousePosInGame, debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix(), debugCamera_->GetWorldPosition(), 1280.0f, 720.0f);
            } else {
                Camera& camera = renderQueue_->GetMainCamera();
                id = staticObjectManager_->SelectObject(mousePosInGame, camera.GetViewMatrix(), camera.GetProjectionMatrix(), camera.GetWorldPosition(), 1280.0f, 720.0f);
            }

            if (id <= -1) {
                selectObject_ = nullptr;
            } else {
                selectObject_ = staticObjectManager_->GetStaticObject(static_cast<uint32_t>(id));
            }
        }
    }

    // ギズモで操作
    if (selectObject_ != nullptr) {

        ImGuizmo::SetOrthographic(false);
        ImVec2 imageMin = ImGui::GetItemRectMin();
        ImVec2 imageSize = ImGui::GetItemRectSize();
        ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

        // 選択しているオブジェクトのワールド行列を取得
        WorldTransform& worldTransform = selectObject_->GetWorldTransform();
        Matrix4x4 objectMatrix = worldTransform.GetWorldMatrix();

        // ギズモで編集するための配列にコピー
        float gizmoMatrix[16];
        std::memcpy(gizmoMatrix, &objectMatrix, sizeof(float) * 16);

        static ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;

        // 簡易的な切り替えUIの例
        //if (ImGui::RadioButton("Translate", currentOperation == ImGuizmo::TRANSLATE)) currentOperation = ImGuizmo::TRANSLATE; ImGui::SameLine();
        //if (ImGui::RadioButton("Rotate", currentOperation == ImGuizmo::ROTATE)) currentOperation = ImGuizmo::ROTATE; ImGui::SameLine();
        //if (ImGui::RadioButton("Scale", currentOperation == ImGuizmo::SCALE)) currentOperation = ImGuizmo::SCALE;
       
        Matrix4x4 viewMat;
        Matrix4x4 projMat;
        if (renderQueue_->GetUseDebugCamera()) {
            viewMat = debugCamera_->GetViewMatrix();
            projMat = debugCamera_->GetProjectionMatrix();
        } else {
            Camera& camera = renderQueue_->GetMainCamera();
            viewMat = camera.GetViewMatrix();
            projMat = camera.GetProjectionMatrix();
        }

        // ギズモを動かしたかどうかの判定
        bool isManipulated = ImGuizmo::Manipulate(
            reinterpret_cast<const float*>(&viewMat),
            reinterpret_cast<const float*>(&projMat),
            currentOperation,
            ImGuizmo::WORLD,
            gizmoMatrix
        );

        if (isManipulated) {
            float translate[3], rotate[3], scale[3];
            ImGuizmo::DecomposeMatrixToComponents(gizmoMatrix, translate, rotate, scale);

            worldTransform.transform_.translate = { translate[0], translate[1], translate[2] };
            constexpr float kToRadian = 3.14159265f / 180.0f;
            worldTransform.transform_.rotate = {
                rotate[0] * kToRadian,
                rotate[1] * kToRadian,
                rotate[2] * kToRadian
            };
            worldTransform.transform_.scale = { scale[0], scale[1], scale[2] };

            // 行列の更新
            worldTransform.UpdateTransformMatrix();
        }
    }
}

void AddObjectBar::AddObjectFromPath(const std::string& filePath) {
    std::filesystem::path path(filePath);
    std::string ext = path.extension().string();

    // 拡張子を小文字に統一
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // モデルファイル以外がドロップされた場合は何もしない
    if (ext != ".obj" && ext != ".gltf") {
        return;
    }

    // 拡張子を含んだファイル名
    std::string modelName = path.filename().string();

    // 同じモデルを複数配置してもオブジェクト名が重複しないように番号を振る
    static int objectCount = 0;
    std::string objectName = modelName + "_" + std::to_string(objectCount++);

    staticObjectManager_->AddObject(objectName, modelName);
}