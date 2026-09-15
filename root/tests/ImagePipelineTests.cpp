#include "ImagePipeline.h"
#include "NativeTest.h"
#include "TestImageFactory.h"

TEST_CASE(Non_square_source_is_centered_without_crop)
{
    const auto loaded = wac::LoadImage(TestImage(L"wide-red-blue.png"));
    REQUIRE_EQ(loaded.succeeded(), true);
    const auto normalized = wac::NormalizeToSquare(*loaded.value);
    REQUIRE_EQ(normalized.size().width, uint32_t{400});
    REQUIRE_EQ(normalized.size().height, uint32_t{400});
    REQUIRE_EQ(normalized.pixel_at(0, 0).a, uint8_t{0});
    REQUIRE_EQ(normalized.pixel_at(20, 200).r, uint8_t{255});
    REQUIRE_EQ(normalized.pixel_at(379, 200).b, uint8_t{255});
}

TEST_CASE(Corrupt_source_returns_corrupt_image_diagnostic)
{
    const auto result = wac::LoadImage(TestImage(L"corrupt.png"));
    REQUIRE_EQ(result.succeeded(), false);
    REQUIRE_EQ(result.diagnostics.front().code, wac::DiagnosticCode::corrupt_image);
}

TEST_CASE(Square_source_is_not_reframed)
{
    const auto loaded = wac::LoadImage(TestImage(L"square-alpha.png"));
    REQUIRE_EQ(loaded.succeeded(), true);
    const auto normalized = wac::NormalizeToSquare(*loaded.value);
    REQUIRE_EQ(normalized.size().width, uint32_t{100});
    REQUIRE_EQ(normalized.size().height, uint32_t{100});
    REQUIRE_EQ(normalized.pixel_at(0, 0).a, uint8_t{128});
}

TEST_CASE(Jpeg_source_converts_to_opaque_rgba)
{
    const auto loaded = wac::LoadImage(TestImage(L"opaque.jpg"));
    REQUIRE_EQ(loaded.succeeded(), true);
    REQUIRE_EQ(loaded.value->pixel_at(20, 10).a, uint8_t{255});
}

TEST_CASE(Exif_rotated_source_has_correct_display_orientation)
{
    const auto loaded = wac::LoadImage(TestImage(L"exif-rotate-90.jpg"));
    REQUIRE_EQ(loaded.succeeded(), true);
    REQUIRE_EQ(loaded.value->size().width, uint32_t{1});
    REQUIRE_EQ(loaded.value->size().height, uint32_t{2});
}

TEST_CASE(Resize_returns_exact_target_dimensions)
{
    const auto loaded = wac::LoadImage(TestImage(L"wide-red-blue.png"));
    REQUIRE_EQ(loaded.succeeded(), true);
    const auto resized = wac::ResizeRgba(*loaded.value, {37, 19});
    REQUIRE_EQ(resized.size().width, uint32_t{37});
    REQUIRE_EQ(resized.size().height, uint32_t{19});
}
