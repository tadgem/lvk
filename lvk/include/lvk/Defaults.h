#pragma once
#include "lvk/Structs.h"

namespace lvk {
namespace defaults {
inline static RasterizationState DefaultRasterState      {VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false};
inline static RasterizationState DefaultRasterStateMSAA  {VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, true};

inline static RasterizationState CullNoneRasterState      {VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false};
inline static RasterizationState CullNoneRasterStateMSAA  {VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, true};

inline static RasterPipelineState DefaultRasterPipelineState { VK_COMPARE_OP_LESS, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
}
}