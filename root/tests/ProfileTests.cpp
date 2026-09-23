#include "NativeTest.h"
#include "StoreMsixProfile.h"

#include <array>
#include <string>
#include <vector>

namespace {
using wac::AssetFormat;
using wac::AssetSpec;
using wac::PixelSize;

struct ExpectedPng {
    std::wstring group;
    std::filesystem::path path;
    PixelSize size;
};

std::vector<ExpectedPng> ExpectedPngAssets() {
    std::vector<ExpectedPng> expected;
    constexpr std::array<uint32_t, 14> app_list_sizes{16, 20, 24, 30, 32, 36, 40,
                                                        48, 60, 64, 72, 80, 96, 256};
    for (const auto size : app_list_sizes) {
        const auto stem = L"Assets/AppList.targetsize-" + std::to_wstring(size);
        expected.push_back({L"AppList default", stem + L".png", {size, size}});
        expected.push_back({L"AppList altform unplated", stem + L"_altform-unplated.png", {size, size}});
        expected.push_back({L"AppList altform light unplated", stem + L"_altform-lightunplated.png", {size, size}});
    }

    const auto add_scaled_group = [&expected](std::wstring const& group, std::wstring const& name,
                                               std::initializer_list<std::pair<uint32_t, uint32_t>> scales,
                                               bool include_base) {
        if (include_base) {
            const auto base = *scales.begin();
            expected.push_back({group, L"Assets/" + name + L".png", {base.second, base.second}});
        }
        for (const auto [scale, pixels] : scales) {
            expected.push_back({group, L"Assets/" + name + L".scale-" + std::to_wstring(scale) + L".png", {pixels, pixels}});
        }
    };

    add_scaled_group(L"Square44", L"AppList", {{100, 44}, {125, 55}, {150, 66}, {200, 88}, {250, 110}, {300, 132}, {400, 176}}, true);
    add_scaled_group(L"Square150", L"Square150x150Logo", {{100, 150}, {125, 188}, {150, 225}, {200, 300}, {250, 375}, {300, 450}, {400, 600}}, true);
    add_scaled_group(L"StoreLogo", L"StoreLogo", {{100, 50}, {125, 63}, {150, 75}, {200, 100}, {400, 200}}, true);
    add_scaled_group(L"MedTile", L"MedTile", {{100, 150}, {125, 188}, {150, 225}, {200, 300}, {400, 600}}, false);
    expected.push_back({L"Wide310", L"Assets/Wide310x150Logo.png", {310, 150}});
    return expected;
}

void RequireValidationFailure(wac::OperationResult const& result, std::filesystem::path const& path) {
    REQUIRE_EQ(result.succeeded(), false);
    REQUIRE_EQ(result.diagnostics.size(), size_t{1});
    REQUIRE_EQ(result.diagnostics.front().code, wac::DiagnosticCode::validation_failure);
    REQUIRE_EQ(result.diagnostics.front().detail, path.wstring());
}
}

TEST_CASE(Store_profile_matches_the_fixed_png_inventory)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto expected = ExpectedPngAssets();
    REQUIRE_EQ(profile.png_assets().size(), size_t{70});
    REQUIRE_EQ(expected.size(), size_t{70});

    for (const auto& expected_asset : expected) {
        const auto* actual = profile.find(expected_asset.path);
        REQUIRE_EQ(actual != nullptr, true);
        REQUIRE_EQ(actual->group, expected_asset.group);
        REQUIRE_EQ(actual->relative_path, expected_asset.path);
        REQUIRE_EQ(actual->size.width, expected_asset.size.width);
        REQUIRE_EQ(actual->size.height, expected_asset.size.height);
        REQUIRE_EQ(actual->format, AssetFormat::png);
        REQUIRE_EQ(profile.contains(expected_asset.path), true);
    }
}

TEST_CASE(Store_profile_uses_only_assets_for_pngs_and_app_icon_for_ico)
{
    const auto profile = wac::StoreMsixProfile::Create();
    for (const auto& asset : profile.png_assets()) {
        REQUIRE_EQ(asset.relative_path.generic_wstring().starts_with(L"Assets/"), true);
        REQUIRE_EQ(asset.format, AssetFormat::png);
    }
    REQUIRE_EQ(profile.ico_asset().relative_path, std::filesystem::path{L"AppIcon.ico"});
    REQUIRE_EQ(profile.ico_asset().format, AssetFormat::ico);
    REQUIRE_EQ(profile.contains(L"AppIcon.ico"), true);
    REQUIRE_EQ(profile.contains(L"Assets/not-planned.png"), false);
    REQUIRE_EQ(profile.find(L"Assets/not-planned.png") == nullptr, true);
}

TEST_CASE(Store_profile_has_expected_group_membership)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto count_group = [&profile](std::wstring const& group) {
        size_t count = 0;
        for (const auto& asset : profile.png_assets()) if (asset.group == group) ++count;
        return count;
    };
    REQUIRE_EQ(count_group(L"AppList default"), size_t{14});
    REQUIRE_EQ(count_group(L"AppList altform unplated"), size_t{14});
    REQUIRE_EQ(count_group(L"AppList altform light unplated"), size_t{14});
    REQUIRE_EQ(count_group(L"Square44"), size_t{8});
    REQUIRE_EQ(count_group(L"Square150"), size_t{8});
    REQUIRE_EQ(count_group(L"StoreLogo"), size_t{6});
    REQUIRE_EQ(count_group(L"MedTile"), size_t{5});
    REQUIRE_EQ(count_group(L"Wide310"), size_t{1});
}

TEST_CASE(Store_profile_validation_accepts_the_fixed_profile)
{
    REQUIRE_EQ(wac::ValidateProfile(wac::StoreMsixProfile::Create()).succeeded(), true);
}

TEST_CASE(Store_profile_validation_rejects_duplicate_paths)
{
    const auto profile = wac::StoreMsixProfile::Create();
    auto assets = std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()};
    assets.push_back(assets.front());
    RequireValidationFailure(wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(std::move(assets))), L"Assets/AppList.targetsize-16.png");
}

TEST_CASE(Store_profile_validation_rejects_zero_dimensions_and_wrong_png_format)
{
    auto profile = wac::StoreMsixProfile::Create();
    auto assets = std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()};
    assets.front().size.width = 0;
    RequireValidationFailure(wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(std::move(assets))), L"Assets/AppList.targetsize-16.png");

    assets = std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()};
    assets.front().size.height = 0;
    RequireValidationFailure(wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(std::move(assets))), L"Assets/AppList.targetsize-16.png");

    assets = std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()};
    assets.front().format = AssetFormat::ico;
    RequireValidationFailure(wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(std::move(assets))), L"Assets/AppList.targetsize-16.png");
}

TEST_CASE(Store_profile_validation_rejects_an_incomplete_png_inventory)
{
    const auto profile = wac::StoreMsixProfile::Create();
    auto assets = std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()};
    assets.pop_back();
    const auto result = wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(std::move(assets)));
    REQUIRE_EQ(result.succeeded(), false);
    REQUIRE_EQ(result.diagnostics.front().code, wac::DiagnosticCode::validation_failure);
}

TEST_CASE(Store_profile_validation_rejects_an_excess_png_inventory)
{
    const auto profile = wac::StoreMsixProfile::Create();
    auto assets = std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()};
    assets.push_back({L"Test", L"Assets/Extra.png", {1, 1}, AssetFormat::png});
    const auto result = wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(std::move(assets)));
    REQUIRE_EQ(result.succeeded(), false);
    REQUIRE_EQ(result.diagnostics.front().code, wac::DiagnosticCode::validation_failure);
}

TEST_CASE(Store_profile_validation_rejects_ico_with_zero_width)
{
    const auto profile = wac::StoreMsixProfile::Create();
    auto ico = profile.ico_asset();
    ico.size.width = 0;
    RequireValidationFailure(wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(
        std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()}, std::move(ico))), L"AppIcon.ico");
}

TEST_CASE(Store_profile_validation_rejects_ico_with_zero_height)
{
    const auto profile = wac::StoreMsixProfile::Create();
    auto ico = profile.ico_asset();
    ico.size.height = 0;
    RequireValidationFailure(wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(
        std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()}, std::move(ico))), L"AppIcon.ico");
}

TEST_CASE(Store_profile_validation_rejects_ico_with_png_format)
{
    const auto profile = wac::StoreMsixProfile::Create();
    auto ico = profile.ico_asset();
    ico.format = AssetFormat::png;
    RequireValidationFailure(wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(
        std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()}, std::move(ico))), L"AppIcon.ico");
}

TEST_CASE(Store_profile_validation_rejects_ico_with_wrong_relative_path)
{
    const auto profile = wac::StoreMsixProfile::Create();
    auto ico = profile.ico_asset();
    ico.relative_path = L"OtherIcon.ico";
    RequireValidationFailure(wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(
        std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()}, std::move(ico))), L"OtherIcon.ico");
}

TEST_CASE(Store_profile_validation_rejects_ico_path_that_collides_with_png)
{
    const auto profile = wac::StoreMsixProfile::Create();
    auto ico = profile.ico_asset();
    ico.relative_path = profile.png_assets().front().relative_path;
    RequireValidationFailure(wac::ValidateProfile(wac::StoreMsixProfileTestAccess::Create(
        std::vector<AssetSpec>{profile.png_assets().begin(), profile.png_assets().end()}, std::move(ico))), L"Assets/AppList.targetsize-16.png");
}
