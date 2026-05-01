#pragma once
#include "RayLibShaderCompiler.h"

namespace GameEngine {

	class StateObjectManager {
	public:
		StateObjectManager() =default;
		~StateObjectManager() = default;

		void Initialize(DXC* dxc);

	private:
		RayLibShaderCompiler rayLibShaderCompiler_;


	};
}