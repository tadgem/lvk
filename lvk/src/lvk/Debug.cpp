#include "lvk/Debug.h"

namespace lvk::debug
{
	VkDebugUtilsLabelEXT CreateDebugLabel(const String& label, Array<float, 4> col)
	{
		VkDebugUtilsLabelEXT l{};
		l.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		l.pNext = nullptr;
		l.pLabelName = label.c_str();
		memcpy(&l.color[0], col.data(), sizeof(col));
		return l;
	}
	void BeginDebugMarker(VkCommandBuffer& cmd, const String& label, Array<float, 4> col)
	{
		auto l = CreateDebugLabel(label, col);
		vkCmdBeginDebugUtilsLabelEXT(cmd, &l);
	}
	void EndDebugMarker(VkCommandBuffer& cmd)
	{
		vkCmdEndDebugUtilsLabelEXT(cmd);
	}
}