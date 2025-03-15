#pragma once
#include "lvk/Structs.h"

namespace lvk
{
	namespace debug {
		VkDebugUtilsLabelEXT CreateDebugLabel(const String& label, Array<float, 4> col = { 1.0f, 1.0f, 1.0f, 1.0f });
		void BeginDebugMarker(VkCommandBuffer& cmd, const String& label, Array<float, 4> col = { 1.0f, 1.0f, 1.0f, 1.0f });
		void EndDebugMarker(VkCommandBuffer& cmd);

	}
}