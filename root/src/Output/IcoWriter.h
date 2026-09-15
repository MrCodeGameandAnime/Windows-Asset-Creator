#pragma once

#include "../ImagePipeline.h"

namespace wac {
OperationResult WriteAppIcon(DecodedImage const& normalized_source,
                             std::filesystem::path const& destination);
}
