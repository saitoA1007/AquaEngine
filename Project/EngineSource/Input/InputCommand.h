#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

#include"InPut.h"

namespace GameEngine {

	// 入力する状態
	enum class InputState {
		KeyPush,
		KeyTrigger,
		MousePush,
		MouseTrigger,
		PadPush,
		PadTrigger,
		PadLeftStick,  // パッドの左スティック
		PadRightStick, // パッドの右スティック
	};

	// コマンドを入力する状態
	struct InputCondition {
		InputState inputState; // どのデバイスでどのように入力するか
		int32_t code; // 押すボタンの種類
		Vector2 direction = { 0.0f,0.0f }; // 方向
		float threshold = 0.2f; // スティックのしきい値
	};

	class InputCommand {
	public:
		~InputCommand() = default;

		/// <summary>
		/// 入力処理を受け取る
		/// </summary>
		/// <param name="input"></param>
		InputCommand(Input* input);

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

		/// <summary>
		/// コマンドを登録する
		/// </summary>
		/// <param name="commandName">コマンドの名前</param>
		/// <param name="conditions">コマンドの振る舞い</param>
		void RegisterCommand(const std::string& commandName, const std::vector<InputCondition>& conditions);

		/// <summary>
		/// コマンドを削除
		/// </summary>
		/// <param name="commandName">コマンド名</param>
		void UnregisterCommand(const std::string& commandName);

		/// <summary>
		/// 指定したコマンドが押されているかをチェック
		/// </summary>
		/// <returns></returns>
		bool IsCommandAcitve(const std::string& commandName) const;

		/// <summary>
		/// 全てのコマンドを削除
		/// </summary>
		void ClearAllCommands();

	private:
		InputCommand(const InputCommand&) = delete;
		const InputCommand& operator=(const InputCommand&) = delete;

		// 入力処理
		Input* input_ = nullptr;

		// 登録されたコマンドと条件
		std::unordered_map<std::string, std::vector<InputCondition>> inputCommands_;
		// 現在の状態
		std::unordered_map<std::string, bool> inputCommandStates_;

	private:

		/// <summary>
		/// コマンドの入力条件チェック
		/// </summary>
		/// <param name="condition"></param>
		/// <returns></returns>
		bool CheckCondition(const InputCondition& condition);
	};
}
