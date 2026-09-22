#include "AssetGenerator.h"
#include "AssetBoardState.h"
#include "NativeTest.h"
#include "Output/Validator.h"
#include "TestImageFactory.h"

#include <array>
#include <chrono>
#include <fstream>
#include <windows.h>

namespace {
std::filesystem::path TempPath(std::wstring const& name) {
    const auto path = std::filesystem::temp_directory_path() / L"WindowsAssetCreator-GenerationTests" /
                      (name + L"-" + std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(path.parent_path());
    return path;
}

uint16_t Read16(std::istream& input) { uint8_t a = input.get(), b = input.get(); return static_cast<uint16_t>(a | (b << 8)); }
uint32_t Read32(std::istream& input) { uint32_t value = 0; for (auto shift : {0, 8, 16, 24}) value |= static_cast<uint32_t>(static_cast<uint8_t>(input.get())) << shift; return value; }
std::vector<std::filesystem::path> ReadZipEntries(std::filesystem::path const& archive) {
    std::ifstream input(archive, std::ios::binary);
    std::vector<std::filesystem::path> entries;
    while (input && Read32(input) == 0x04034b50) {
        input.ignore(14);
        const auto data_size = Read32(input);
        input.ignore(4);
        const auto name_size = Read16(input);
        const auto extra_size = Read16(input);
        std::string name(name_size, '\0');
        input.read(name.data(), name.size());
        input.ignore(extra_size + data_size);
        entries.emplace_back(std::filesystem::path{name});
    }
    return entries;
}

bool Contains(std::vector<std::filesystem::path> const& entries, std::filesystem::path const& target) {
    for (const auto& entry : entries) if (entry.generic_wstring() == target.generic_wstring()) return true;
    return false;
}

std::string ReadBytes(std::filesystem::path const& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}

wac::GenerationSession ReadySession() {
    auto result = wac::AssetGenerator{}.Generate(TestImage(L"wide-red-blue.png"));
    REQUIRE_EQ(result.succeeded(), true);
    return std::move(*result.value);
}

wac::SourcePresentation ReadySourcePresentation() {
    return {L"wide-red-blue.png", {400, 200}};
}
}

TEST_CASE(Generation_creates_valid_69_png_and_ico_session)
{
    auto session = ReadySession();
    REQUIRE_EQ(session.preview_assets().size(), size_t{70});
    REQUIRE_EQ(wac::ValidateStagedAssets(session.profile(), session.staging_root()).succeeded(), true);
}

TEST_CASE(Export_zip_contains_only_assets_and_appicon)
{
    auto session = ReadySession();
    const auto zip = TempPath(L"Windows-Assets.zip");
    REQUIRE_EQ(session.ExportZip(zip).succeeded(), true);
    const auto entries = ReadZipEntries(zip);
    REQUIRE_EQ(entries.size(), size_t{70});
    REQUIRE_EQ(Contains(entries, L"Assets/AppList.targetsize-16.png"), true);
    REQUIRE_EQ(Contains(entries, L"AppIcon.ico"), true);
    REQUIRE_EQ(Contains(entries, TestImage(L"wide-red-blue.png")), false);
}

TEST_CASE(Corrupt_source_does_not_create_ready_session)
{
    const auto result = wac::AssetGenerator{}.Generate(TestImage(L"corrupt.png"));
    REQUIRE_EQ(result.succeeded(), false);
}

TEST_CASE(Export_failure_keeps_existing_user_zip_unchanged)
{
    auto session = ReadySession();
    const auto zip = TempPath(L"existing.zip");
    { std::ofstream existing(zip, std::ios::binary); existing << "keep"; }
    REQUIRE_EQ(session.ExportZip(zip / L"child.zip").succeeded(), false);
    REQUIRE_EQ(ReadBytes(zip), std::string("keep"));
}

TEST_CASE(Session_cleanup_removes_only_its_own_staging_directory)
{
    const auto sibling = TempPath(L"sibling");
    std::filesystem::create_directories(sibling);
    std::filesystem::path staging;
    {
        auto session = ReadySession();
        staging = session.staging_root();
        REQUIRE_EQ(std::filesystem::exists(staging), true);
    }
    REQUIRE_EQ(std::filesystem::exists(staging), false);
    REQUIRE_EQ(std::filesystem::exists(sibling), true);
}

TEST_CASE(Zip_manifest_rejects_missing_planned_file)
{
    auto session = ReadySession();
    std::filesystem::remove(session.staging_root() / session.profile().png_assets().front().relative_path);
    REQUIRE_EQ(session.ExportZip(TempPath(L"missing.zip")).succeeded(), false);
}

TEST_CASE(Move_constructed_session_retains_transferred_staging_directory)
{
    std::filesystem::path staging;
    std::optional<wac::GenerationSession> owner;
    {
        auto source = ReadySession();
        staging = source.staging_root();
        owner.emplace(std::move(source));
        REQUIRE_EQ(source.staging_root().empty(), true);
        REQUIRE_EQ(std::filesystem::exists(staging), true);
    }
    REQUIRE_EQ(std::filesystem::exists(staging), true);
    owner.reset();
    REQUIRE_EQ(std::filesystem::exists(staging), false);
}

TEST_CASE(Move_assignment_cleans_previous_owned_staging_directory)
{
    auto destination = ReadySession();
    const auto previous_staging = destination.staging_root();
    std::filesystem::path transferred_staging;
    {
        auto source = ReadySession();
        transferred_staging = source.staging_root();
        destination = std::move(source);
        REQUIRE_EQ(source.staging_root().empty(), true);
        REQUIRE_EQ(std::filesystem::exists(previous_staging), false);
        REQUIRE_EQ(std::filesystem::exists(transferred_staging), true);
    }
    REQUIRE_EQ(std::filesystem::exists(transferred_staging), true);
}

TEST_CASE(Live_generation_sessions_use_distinct_staging_roots)
{
    auto first = ReadySession();
    auto second = ReadySession();
    auto third = ReadySession();
    REQUIRE_EQ(first.staging_root() == second.staging_root(), false);
    REQUIRE_EQ(first.staging_root() == third.staging_root(), false);
    REQUIRE_EQ(second.staging_root() == third.staging_root(), false);
    REQUIRE_EQ(std::filesystem::exists(first.staging_root()), true);
    REQUIRE_EQ(std::filesystem::exists(second.staging_root()), true);
    REQUIRE_EQ(std::filesystem::exists(third.staging_root()), true);
}

TEST_CASE(Export_successfully_replaces_existing_user_zip)
{
    auto session = ReadySession();
    const auto zip = TempPath(L"replacement.zip");
    { std::ofstream existing(zip, std::ios::binary); existing << "sentinel"; }

    REQUIRE_EQ(session.ExportZip(zip).succeeded(), true);
    const auto entries = ReadZipEntries(zip);
    REQUIRE_EQ(entries.size(), size_t{70});
    REQUIRE_EQ(ReadBytes(zip).find("sentinel") == std::string::npos, true);
    REQUIRE_EQ(std::filesystem::exists(zip.wstring() + L".tmp"), false);
}

TEST_CASE(Export_replacement_failure_cleans_temporary_file)
{
    auto session = ReadySession();
    const auto zip = TempPath(L"replacement-failure.zip");
    { std::ofstream existing(zip, std::ios::binary); existing << "keep"; }
    REQUIRE_EQ(SetFileAttributesW(zip.c_str(), FILE_ATTRIBUTE_READONLY) != 0, true);

    const auto result = session.ExportZip(zip);
    REQUIRE_EQ(SetFileAttributesW(zip.c_str(), FILE_ATTRIBUTE_NORMAL) != 0, true);
    REQUIRE_EQ(result.succeeded(), false);
    REQUIRE_EQ(ReadBytes(zip), std::string("keep"));
    REQUIRE_EQ(std::filesystem::exists(zip.wstring() + L".tmp"), false);
}

TEST_CASE(Board_state_is_ready_only_after_valid_generation)
{
    wac::AssetBoardState state;
    REQUIRE_EQ(state.phase(), wac::BoardPhase::idle);
    REQUIRE_EQ(state.can_save(), false);

    state.BeginGeneration();
    REQUIRE_EQ(state.phase(), wac::BoardPhase::processing);
    REQUIRE_EQ(state.can_save(), false);

    state.CompleteGeneration(ReadySession(), ReadySourcePresentation());
    REQUIRE_EQ(state.phase(), wac::BoardPhase::ready);
    REQUIRE_EQ(state.can_save(), true);
    REQUIRE_EQ(state.session().has_value(), true);
}

TEST_CASE(Board_state_groups_every_generated_preview)
{
    wac::AssetBoardState state;
    state.CompleteGeneration(ReadySession(), ReadySourcePresentation());

    size_t asset_count = 0;
    for (const auto& group : state.groups()) {
        REQUIRE_EQ(group.title.empty(), false);
        asset_count += group.assets.size();
    }

    REQUIRE_EQ(state.groups().size(), size_t{8});
    REQUIRE_EQ(asset_count, size_t{70});
    REQUIRE_EQ(state.groups().front().title, std::wstring{L"AppList default"});
    REQUIRE_EQ(state.groups().back().title, std::wstring{L"AppIcon"});
}

TEST_CASE(Board_state_retains_accepted_source_presentation)
{
    wac::AssetBoardState state;
    state.CompleteGeneration(ReadySession(), {L"master-logo.png", {1024, 768}});

    REQUIRE_EQ(state.source().has_value(), true);
    REQUIRE_EQ(state.source()->name, std::wstring{L"master-logo.png"});
    REQUIRE_EQ(state.source()->dimensions.width, uint32_t{1024});
    REQUIRE_EQ(state.source()->dimensions.height, uint32_t{768});
}

TEST_CASE(Board_state_second_source_replacement_updates_source_presentation)
{
    wac::AssetBoardState state;
    state.BeginGeneration();
    state.CompleteGeneration(ReadySession(), {L"source-a.png", {640, 480}});

    state.BeginGeneration();
    state.CompleteGeneration(ReadySession(), {L"source-b.jpg", {1280, 720}});

    REQUIRE_EQ(state.phase(), wac::BoardPhase::ready);
    REQUIRE_EQ(state.can_save(), true);
    REQUIRE_EQ(state.source().has_value(), true);
    REQUIRE_EQ(state.source()->name, std::wstring{L"source-b.jpg"});
    REQUIRE_EQ(state.source()->dimensions.width, uint32_t{1280});
    REQUIRE_EQ(state.source()->dimensions.height, uint32_t{720});
}

TEST_CASE(Reset_clears_source_presentation)
{
    wac::AssetBoardState state;
    state.CompleteGeneration(ReadySession(), {L"master-logo.png", {1024, 768}});

    state.Reset();

    REQUIRE_EQ(state.source().has_value(), false);
}

TEST_CASE(Reset_from_ready_returns_to_idle)
{
    wac::AssetBoardState state;
    state.CompleteGeneration(ReadySession(), ReadySourcePresentation());
    const auto staging_root = state.session()->staging_root();
    REQUIRE_EQ(std::filesystem::exists(staging_root), true);

    state.Reset();

    REQUIRE_EQ(state.phase(), wac::BoardPhase::idle);
    REQUIRE_EQ(state.can_save(), false);
    REQUIRE_EQ(state.session().has_value(), false);
    REQUIRE_EQ(state.groups().empty(), true);
    REQUIRE_EQ(state.diagnostics().empty(), true);
    REQUIRE_EQ(std::filesystem::exists(staging_root), false);
}

TEST_CASE(Reset_from_error_returns_to_idle)
{
    wac::AssetBoardState state;
    state.CompleteFailure({{wac::Severity::error, wac::DiagnosticCode::corrupt_image,
                           L"The source image is corrupt.", L"corrupt.png"}});

    state.Reset();

    REQUIRE_EQ(state.phase(), wac::BoardPhase::idle);
    REQUIRE_EQ(state.can_save(), false);
    REQUIRE_EQ(state.session().has_value(), false);
    REQUIRE_EQ(state.diagnostics().empty(), true);
}

TEST_CASE(Board_state_allows_reset_only_when_not_busy)
{
    wac::AssetBoardState state;
    REQUIRE_EQ(state.can_reset(), false);
    state.BeginGeneration();
    REQUIRE_EQ(state.can_reset(), false);
    state.CompleteFailure({{wac::Severity::error, wac::DiagnosticCode::unsupported_image,
                           L"Choose one PNG or JPEG image.", L"notes.txt"}});
    REQUIRE_EQ(state.can_reset(), true);
    state.Reset();
    state.BeginGeneration();
    state.CompleteGeneration(ReadySession(), ReadySourcePresentation());
    REQUIRE_EQ(state.can_reset(), true);
    state.BeginSave();
    REQUIRE_EQ(state.can_reset(), false);
    state.CompleteSaveCancelled();
    REQUIRE_EQ(state.can_reset(), true);
}

TEST_CASE(Board_state_allows_replacement_intake_only_when_ready)
{
    wac::AssetBoardState state;
    REQUIRE_EQ(state.can_replace_source(), false);

    state.BeginGeneration();
    REQUIRE_EQ(state.can_replace_source(), false);

    state.CompleteGeneration(ReadySession(), ReadySourcePresentation());
    REQUIRE_EQ(state.can_replace_source(), true);

    state.BeginSave();
    REQUIRE_EQ(state.can_replace_source(), false);

    state.CompleteSaveCancelled();
    REQUIRE_EQ(state.can_replace_source(), true);

    state.CompleteFailure({{wac::Severity::error, wac::DiagnosticCode::corrupt_image,
                            L"The source image could not be decoded.", L"corrupt.png"}});
    REQUIRE_EQ(state.can_replace_source(), false);
}

TEST_CASE(Board_state_keeps_save_disabled_after_corrupt_source)
{
    wac::AssetBoardState state;
    state.CompleteFailure({{wac::Severity::error, wac::DiagnosticCode::corrupt_image,
                           L"The source image is corrupt.", L"corrupt.png"}});

    REQUIRE_EQ(state.phase(), wac::BoardPhase::error);
    REQUIRE_EQ(state.can_save(), false);
    REQUIRE_EQ(state.session().has_value(), false);
    REQUIRE_EQ(state.diagnostics().front().code, wac::DiagnosticCode::corrupt_image);
}

TEST_CASE(Cancelled_save_returns_board_to_ready_state)
{
    wac::AssetBoardState state;
    state.CompleteGeneration(ReadySession(), ReadySourcePresentation());
    state.BeginSave();
    state.CompleteSaveCancelled();
    REQUIRE_EQ(state.phase(), wac::BoardPhase::ready);
    REQUIRE_EQ(state.can_save(), true);
}

TEST_CASE(Successful_save_returns_board_to_ready_state)
{
    wac::AssetBoardState state;
    state.CompleteGeneration(ReadySession(), ReadySourcePresentation());
    state.BeginSave();
    state.CompleteSaveSuccess();
    REQUIRE_EQ(state.phase(), wac::BoardPhase::ready);
    REQUIRE_EQ(state.can_save(), true);
    REQUIRE_EQ(state.session().has_value(), true);
}

TEST_CASE(Failed_export_preserves_the_ready_session)
{
    wac::AssetBoardState state;
    state.CompleteGeneration(ReadySession(), ReadySourcePresentation());
    state.BeginSave();
    state.CompleteSaveFailure({wac::Severity::error, wac::DiagnosticCode::zip_failure,
                               L"The ZIP could not be saved.", L"output.zip"});
    REQUIRE_EQ(state.phase(), wac::BoardPhase::ready);
    REQUIRE_EQ(state.can_save(), true);
    REQUIRE_EQ(state.session().has_value(), true);
    REQUIRE_EQ(state.diagnostics().front().code, wac::DiagnosticCode::zip_failure);
}
