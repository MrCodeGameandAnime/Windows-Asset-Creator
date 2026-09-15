#include "StoreMsixProfile.h"

#include <array>
#include <string>
#include <unordered_set>

namespace wac {
namespace {
constexpr std::array<uint32_t, 14> app_list_sizes{16, 20, 24, 30, 32, 36, 40,
                                                    48, 60, 64, 72, 80, 96, 256};

struct ScaleSize {
    uint32_t scale;
    uint32_t pixels;
};

constexpr std::array<ScaleSize, 7> square44_scales{{{100, 44}, {125, 55}, {150, 66}, {200, 88},
                                                      {250, 110}, {300, 132}, {400, 176}}};
constexpr std::array<ScaleSize, 7> square150_scales{{{100, 150}, {125, 188}, {150, 225}, {200, 300},
                                                       {250, 375}, {300, 450}, {400, 600}}};
constexpr std::array<ScaleSize, 5> store_logo_scales{{{100, 50}, {125, 63}, {150, 75}, {200, 100}, {400, 200}}};
constexpr std::array<ScaleSize, 5> med_tile_scales{{{100, 150}, {125, 188}, {150, 225}, {200, 300}, {400, 600}}};

AssetSpec MakePng(std::wstring group, std::wstring path, uint32_t pixels) {
    return {std::move(group), std::filesystem::path{std::move(path)}, {pixels, pixels}, AssetFormat::png};
}

template <size_t Count>
void AddScaledAssets(std::vector<AssetSpec>& assets, std::wstring const& group, std::wstring const& name,
                     std::array<ScaleSize, Count> const& scales, bool include_base) {
    if (include_base) {
        assets.push_back(MakePng(group, L"Assets/" + name + L".png", scales.front().pixels));
    }
    for (const auto scale : scales) {
        assets.push_back(MakePng(group, L"Assets/" + name + L".scale-" + std::to_wstring(scale.scale) + L".png",
                                 scale.pixels));
    }
}

OperationResult ValidationFailure(std::wstring detail) {
    return {{{Severity::error, DiagnosticCode::validation_failure,
              L"Store/MSIX profile validation failed.", std::move(detail)}}};
}
}

StoreMsixProfile::StoreMsixProfile(std::vector<AssetSpec> png_assets, AssetSpec ico_asset)
    : png_assets_(std::move(png_assets)), ico_asset_(std::move(ico_asset)) {}

StoreMsixProfile StoreMsixProfile::Create() {
    std::vector<AssetSpec> png_assets;
    png_assets.reserve(69);

    for (const auto size : app_list_sizes) {
        const auto stem = L"Assets/AppList.targetsize-" + std::to_wstring(size);
        png_assets.push_back(MakePng(L"AppList default", stem + L".png", size));
        png_assets.push_back(MakePng(L"AppList altform unplated", stem + L"_altform-unplated.png", size));
        png_assets.push_back(MakePng(L"AppList altform light unplated", stem + L"_altform-lightunplated.png", size));
    }

    AddScaledAssets(png_assets, L"Square44", L"Square44x44Logo", square44_scales, true);
    AddScaledAssets(png_assets, L"Square150", L"Square150x150Logo", square150_scales, true);
    AddScaledAssets(png_assets, L"StoreLogo", L"StoreLogo", store_logo_scales, true);
    AddScaledAssets(png_assets, L"MedTile", L"MedTile", med_tile_scales, false);

    return StoreMsixProfile{std::move(png_assets), {L"AppIcon", L"AppIcon.ico", {256, 256}, AssetFormat::ico}};
}

std::span<AssetSpec const> StoreMsixProfile::png_assets() const noexcept {
    return png_assets_;
}

AssetSpec const& StoreMsixProfile::ico_asset() const noexcept {
    return ico_asset_;
}

AssetSpec const* StoreMsixProfile::find(std::filesystem::path const& relative_path) const noexcept {
    for (const auto& asset : png_assets_) {
        if (asset.relative_path == relative_path) return &asset;
    }
    return ico_asset_.relative_path == relative_path ? &ico_asset_ : nullptr;
}

bool StoreMsixProfile::contains(std::filesystem::path const& relative_path) const noexcept {
    return find(relative_path) != nullptr;
}

OperationResult ValidateProfile(StoreMsixProfile const& profile) {
    std::unordered_set<std::wstring> paths;
    for (const auto& asset : profile.png_assets()) {
        const auto path = asset.relative_path.generic_wstring();
        if (!paths.insert(path).second) return ValidationFailure(asset.relative_path.wstring());
        if (asset.size.width == 0 || asset.size.height == 0) return ValidationFailure(asset.relative_path.wstring());
        if (asset.format != AssetFormat::png) return ValidationFailure(asset.relative_path.wstring());
    }
    if (profile.png_assets().size() != 69) {
        return ValidationFailure(L"Expected 69 PNG assets but found " + std::to_wstring(profile.png_assets().size()) + L".");
    }
    return {};
}
}
