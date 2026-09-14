#include "NativeTest.h"
#include "StoreMsixProfile.h"

TEST_CASE(Store_profile_has_expected_asset_count)
{
    const auto profile = wac::StoreMsixProfile::Create();
    REQUIRE_EQ(profile.png_assets().size(), size_t{69});
    REQUIRE_EQ(profile.ico_asset().relative_path, std::filesystem::path{L"AppIcon.ico"});
}
