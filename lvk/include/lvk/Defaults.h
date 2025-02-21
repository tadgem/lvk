#pragma once
#include "lvk/Structs.h"

namespace lvk {
namespace defaults {
inline static RasterState DefaultRasterState      {VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false};
inline static RasterState DefaultRasterStateMSAA  {VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, true};

inline static RasterState CullNoneRasterState      {VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false};
inline static RasterState CullNoneRasterStateMSAA  {VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, true};

}
}