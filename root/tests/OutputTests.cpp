#include "ImagePipeline.h"
#include "NativeTest.h"
#include "Output/IcoWriter.h"
#include "Output/PngEncoder.h"
#include "Output/Validator.h"
#include "StoreMsixProfile.h"
#include "TestImageFactory.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <thread>
#include <vector>

namespace {
std::filesystem::path TestRoot(std::wstring const& name) {
    const auto root = std::filesystem::temp_directory_path() / L"WindowsAssetCreator-OutputTests" /
                      (name + L"-" + std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    return root;
}

uint16_t ReadU16(std::istream& input) {
    std::array<uint8_t, 2> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
}

void WriteU16(std::ostream& output, uint16_t value) {
    output.put(static_cast<char>(value));
    output.put(static_cast<char>(value >> 8));
}

void WriteU32(std::ostream& output, uint32_t value) {
    for (const auto shift : {0, 8, 16, 24}) output.put(static_cast<char>(value >> shift));
}

void WriteIcoDirectory(std::filesystem::path const& path, uint32_t byte_count, uint32_t offset,
                       std::vector<uint8_t> const& payload = {}) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    constexpr std::array<uint32_t, 5> sizes{16, 24, 32, 48, 256};
    WriteU16(output, 0); WriteU16(output, 1); WriteU16(output, 5);
    for (const auto size : sizes) {
        output.put(static_cast<char>(size == 256 ? 0 : size));
        output.put(static_cast<char>(size == 256 ? 0 : size));
        output.put(0); output.put(0); WriteU16(output, 1); WriteU16(output, 32);
        WriteU32(output, byte_count); WriteU32(output, offset);
    }
    output.write(reinterpret_cast<char const*>(payload.data()), payload.size());
}

std::vector<uint32_t> ReadIcoDirectory(std::filesystem::path const& path) {
    std::ifstream input(path, std::ios::binary);
    REQUIRE_EQ(input.good(), true);
    REQUIRE_EQ(ReadU16(input), uint16_t{0});
    REQUIRE_EQ(ReadU16(input), uint16_t{1});
    const auto count = ReadU16(input);
    std::vector<uint32_t> sizes;
    for (uint16_t index = 0; index < count; ++index) {
        const auto width = static_cast<uint8_t>(input.get());
        const auto height = static_cast<uint8_t>(input.get());
        input.ignore(6);
        sizes.push_back(width == 0 && height == 0 ? 256U : width);
        input.ignore(8);
    }
    return sizes;
}

wac::DecodedImage Source() {
    const auto source = wac::LoadImage(TestImage(L"wide-red-blue.png"));
    REQUIRE_EQ(source.succeeded(), true);
    return wac::NormalizeToSquare(*source.value);
}

void WriteCompleteProfile(wac::StoreMsixProfile const& profile, std::filesystem::path const& root) {
    const auto source = Source();
    for (const auto& asset : profile.png_assets()) {
        REQUIRE_EQ(wac::EncodePng(source, root / asset.relative_path, asset.size).succeeded(), true);
    }
    REQUIRE_EQ(wac::WriteAppIcon(source, root / profile.ico_asset().relative_path).succeeded(), true);
}
}

TEST_CASE(Png_encoder_writes_exact_rgba_dimensions)
{
    const auto destination = TestRoot(L"png") / L"square44.png";
    REQUIRE_EQ(wac::EncodePng(Source(), destination, {44, 44}).succeeded(), true);
    const auto output = wac::LoadImage(destination);
    REQUIRE_EQ(output.succeeded(), true);
    REQUIRE_EQ(output.value->size().width, uint32_t{44});
    REQUIRE_EQ(output.value->size().height, uint32_t{44});
    REQUIRE_EQ(output.value->pixel_at(0, 0).a, uint8_t{0});
}

TEST_CASE(Ico_writer_contains_five_requested_sizes)
{
    const auto destination = TestRoot(L"ico") / L"AppIcon.ico";
    REQUIRE_EQ(wac::WriteAppIcon(Source(), destination).succeeded(), true);
    const auto sizes = ReadIcoDirectory(destination);
    REQUIRE_EQ(sizes, std::vector<uint32_t>({16, 24, 32, 48, 256}));
}

TEST_CASE(Validator_accepts_complete_profile_output)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto root = TestRoot(L"complete");
    WriteCompleteProfile(profile, root);
    REQUIRE_EQ(wac::ValidateStagedAssets(profile, root).succeeded(), true);
}

TEST_CASE(Validator_rejects_wrong_png_dimensions)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto root = TestRoot(L"wrong-size");
    WriteCompleteProfile(profile, root);
    REQUIRE_EQ(wac::EncodePng(Source(), root / profile.png_assets().front().relative_path, {1, 1}).succeeded(), true);
    REQUIRE_EQ(wac::ValidateStagedAssets(profile, root).succeeded(), false);
}

TEST_CASE(Validator_rejects_missing_asset)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto root = TestRoot(L"missing");
    WriteCompleteProfile(profile, root);
    std::filesystem::remove(root / profile.png_assets().front().relative_path);
    REQUIRE_EQ(wac::ValidateStagedAssets(profile, root).succeeded(), false);
}

TEST_CASE(Validator_rejects_malformed_ico)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto root = TestRoot(L"malformed-ico");
    WriteCompleteProfile(profile, root);
    std::ofstream corrupt(root / profile.ico_asset().relative_path, std::ios::binary | std::ios::trunc);
    corrupt << "not an ico";
    corrupt.close();
    REQUIRE_EQ(wac::ValidateStagedAssets(profile, root).succeeded(), false);
}

TEST_CASE(Validator_rejects_unplanned_staged_file)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto root = TestRoot(L"unexpected");
    WriteCompleteProfile(profile, root);
    std::ofstream unexpected(root / L"unexpected.png", std::ios::binary | std::ios::trunc);
    unexpected << "unexpected";
    unexpected.close();
    REQUIRE_EQ(wac::ValidateStagedAssets(profile, root).succeeded(), false);
}

TEST_CASE(Validator_rejects_ico_directory_without_payload_data)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto root = TestRoot(L"ico-empty");
    WriteCompleteProfile(profile, root);
    WriteIcoDirectory(root / profile.ico_asset().relative_path, 8, 86);
    REQUIRE_EQ(wac::ValidateStagedAssets(profile, root).succeeded(), false);
}

TEST_CASE(Validator_rejects_ico_payload_outside_file)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto root = TestRoot(L"ico-outside");
    WriteCompleteProfile(profile, root);
    WriteIcoDirectory(root / profile.ico_asset().relative_path, 8, 4096);
    REQUIRE_EQ(wac::ValidateStagedAssets(profile, root).succeeded(), false);
}

TEST_CASE(Validator_rejects_ico_non_png_payload)
{
    const auto profile = wac::StoreMsixProfile::Create();
    const auto root = TestRoot(L"ico-not-png");
    WriteCompleteProfile(profile, root);
    WriteIcoDirectory(root / profile.ico_asset().relative_path, 8, 86,
                      {0, 1, 2, 3, 4, 5, 6, 7});
    REQUIRE_EQ(wac::ValidateStagedAssets(profile, root).succeeded(), false);
}

TEST_CASE(Png_encoder_wic_failure_retains_destination_path)
{
    const auto destination = TestRoot(L"png-error") / L"bad.png";
    const auto result = wac::EncodePng(Source(), destination, {0, 0});
    REQUIRE_EQ(result.succeeded(), false);
    REQUIRE_EQ(result.diagnostics.front().detail.find(destination.wstring()) != std::wstring::npos, true);
}

TEST_CASE(Png_encoder_succeeds_from_sta_thread)
{
    const auto destination = TestRoot(L"sta") / L"sta.png";
    bool encoded = false;
    bool dimensions_match = false;
    std::thread thread([&] {
        const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(initialized)) return;
        const auto source = wac::LoadImage(TestImage(L"wide-red-blue.png"));
        if (source.succeeded()) {
            const auto result = wac::EncodePng(wac::NormalizeToSquare(*source.value), destination, {44, 44});
            encoded = result.succeeded();
            const auto output = wac::LoadImage(destination);
            dimensions_match = output.succeeded() && output.value->size().width == 44 && output.value->size().height == 44;
        }
        CoUninitialize();
    });
    thread.join();
    REQUIRE_EQ(encoded, true);
    REQUIRE_EQ(dimensions_match, true);
}
