#pragma once

#include "AssetTypes.h"

#include <span>
#include <utility>
#include <vector>

namespace wac {
class StoreMsixProfileTestAccess;

class StoreMsixProfile final {
public:
    static StoreMsixProfile Create();

    std::span<AssetSpec const> png_assets() const noexcept;
    AssetSpec const& ico_asset() const noexcept;
    AssetSpec const* find(std::filesystem::path const& relative_path) const noexcept;
    bool contains(std::filesystem::path const& relative_path) const noexcept;

private:
    friend class StoreMsixProfileTestAccess;

    StoreMsixProfile(std::vector<AssetSpec> png_assets, AssetSpec ico_asset);

    std::vector<AssetSpec> png_assets_;
    AssetSpec ico_asset_;
};

OperationResult ValidateProfile(StoreMsixProfile const& profile);

#if defined(WAC_PROFILE_TESTING)
class StoreMsixProfileTestAccess final {
public:
    static StoreMsixProfile Create(std::vector<AssetSpec> png_assets) {
        return StoreMsixProfile{std::move(png_assets),
                                {L"AppIcon", L"AppIcon.ico", {256, 256}, AssetFormat::ico}};
    }
};
#endif
}
