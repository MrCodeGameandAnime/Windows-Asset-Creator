#pragma once

#include "../StoreMsixProfile.h"

namespace wac {
OperationResult ValidateStagedAssets(StoreMsixProfile const& profile,
                                     std::filesystem::path const& staging_root);
}
