#pragma once

#include "../ImagePipeline.h"

namespace wac {
OperationResult EncodePng(DecodedImage const& normalized_source,
                          std::filesystem::path const& destination,
                          PixelSize target_size);
}
