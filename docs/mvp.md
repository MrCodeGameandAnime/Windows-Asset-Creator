# Windows Asset Creator MVP Implementation Plan

> **Execution policy:** Implement this plan sequentially with one primary agent. Do not spawn subagents, reviewer agents, parallel workers, or delegated investigations unless the user explicitly requests a narrowly defined delegation. Complete one numbered task at a time, verify it with the strongest available build/test evidence, report briefly, and stop at the task gate for external review.

**Goal:** Build a packaged WinUI 3/C++/WinRT Windows utility that turns one raster source image into a validated Microsoft Store/MSIX ZIP asset package.

**Architecture:** A UI-independent C++20 static library in `root/src` owns the Store/MSIX asset plan, WIC image pipeline, staging session, validation, ICO creation, and ZIP writing. A packaged WinUI 3/C++/WinRT application in `root/windows` owns drag/drop, the scrollable asset board, Windows Save As, and Explorer reveal. The UI invokes the core asynchronously and never writes output itself.

**Tech Stack:** C++20, MSBuild-based native C++ projects compatible with the WinUI 3 Blank App (Packaged) structure, Windows App SDK/WinUI 3, C++/WinRT, Windows Imaging Component, standard-library filesystem, Win32 Shell APIs, and a self-contained native C++ test executable. Visual Studio IDE is optional; the canonical build path is command-line MSBuild.

**Spec:** `docs/design.md`

## Agent Behavior

This plan defines product work, not a multi-agent process. The primary agent owns implementation from start to finish.

- Do not spawn subagents for environment discovery, dependency checks, implementation, review, re-review, or documentation unless the user explicitly asks for a narrowly scoped delegation.
- Do not begin, inspect, plan, delegate, or prepare the next numbered task before the current task has been externally approved.
- Do not create task briefs, execution reports, evidence ledgers, review reports, postmortems, or other process Markdown. Terminal/build/test output is sufficient evidence unless the product itself requires a file artifact.
- Do not modify `.superpowers/` or create additional process scaffolding as part of implementation.
- Do not automatically commit. The user controls commit timing unless they explicitly instruct otherwise.
- Do not expand scope to tooling, repository hygiene, IDE configuration, documentation, or developer-experience work unless it is required by the current task.
- If a required tool is missing, confirm that directly, report the blocker once, and stop if it prevents meaningful verification. Do not form a review/delegation chain around an environment prerequisite.
- Prefer compiler, linker, test-runner, and runtime results over self-review claims.

## Verification Policy

Tests specify required behavior, but the plan does not require manufacturing artificial intermediate failures just to demonstrate red-green ceremony. A task should finish in a buildable, testable state whenever reasonably possible.

For each numbered task:

1. Implement only the current task.
2. Build everything that is expected to build at that stage.
3. Run every test that is expected to pass at that stage.
4. Fix failures caused by the current task before reporting completion.
5. If verification is blocked by a missing external prerequisite, report exactly what is unverified and why.
6. Return a short completion summary containing files changed, build result, test result, and any real unresolved concern.
7. **STOP and wait for external review.**

Do not treat work intentionally deferred to a later numbered task as a current failure requiring investigation. Do not continue into the next task because the current task appears successful.

---

## Global Constraints

- Preserve the top-level `root/windows`, `root/src`, `root/src/Output`, and `root/tests` layout.
- All automated tests and their fixtures live in `root/tests`; do not put test code below `root/src` or `root/windows`.
- The product is C++20, local-only, Windows-first, and WinUI 3/C++/WinRT; it has no .NET runtime dependency, Python runtime, cloud service, account, telemetry, or CLI in the MVP.
- Use WIC for supported image decode, RGBA conversion, scaling, and PNG encoding.
- Do not crop source artwork by default. Center non-square sources on a transparent square before rendering.
- Generate exactly 69 PNG files and `AppIcon.ico`; package only `Assets/` and `AppIcon.ico` in the ZIP.
- Keep the Store/MSIX profile internal. Do not expose a profile selector.
- Save As occurs only after a valid temporary generation session is ready. Never delete or overwrite a user-selected output folder.
- Never retain a product-specific `HeadsUp` filename.
- Keep the repository free of execution-only evidence/report files.
- Keep Visual Studio IDE optional; project files may be Visual Studio-compatible because MSBuild consumes them, but implementation must not require using the IDE.

---

## Planned File Structure

```text
root/
  WindowsAssetCreator.sln
  windows/
    WindowsAssetCreator.vcxproj
    App.xaml / App.xaml.h / App.xaml.cpp
    MainWindow.xaml / MainWindow.xaml.h / MainWindow.xaml.cpp
    AssetBoardState.h / AssetBoardState.cpp
    AssetBoardViewModel.h / AssetBoardViewModel.cpp
    ExplorerReveal.h / ExplorerReveal.cpp
  src/
    AssetCore.vcxproj
    AssetTypes.h
    StoreMsixProfile.h / StoreMsixProfile.cpp
    ImagePipeline.h / ImagePipeline.cpp
    AssetGenerator.h / AssetGenerator.cpp
    Output/
      PngEncoder.h / PngEncoder.cpp
      IcoWriter.h / IcoWriter.cpp
      Validator.h / Validator.cpp
      StagingSession.h / StagingSession.cpp
      ZipWriter.h / ZipWriter.cpp
  tests/
    AssetCoreTests.vcxproj
    TestImages/
    ProfileTests.cpp
    ImagePipelineTests.cpp
    OutputTests.cpp
    GenerationSessionTests.cpp
    TestImageFactory.h / TestImageFactory.cpp
    NativeTest.h / NativeTest.cpp
```

## Task 1: Create the native solution, project boundaries, and test harness

**Files:**
- Create: `root/WindowsAssetCreator.sln`
- Create: `root/windows/WindowsAssetCreator.vcxproj`, `root/windows/App.xaml`, `root/windows/App.xaml.h`, `root/windows/App.xaml.cpp`, `root/windows/MainWindow.xaml`, `root/windows/MainWindow.xaml.h`, `root/windows/MainWindow.xaml.cpp`
- Create: `root/src/AssetCore.vcxproj`, `root/src/AssetTypes.h`
- Create: `root/tests/AssetCoreTests.vcxproj`, `root/tests/NativeTest.h`, `root/tests/NativeTest.cpp`
- Modify: `root/instructions.txt`, `root/windows/instructions.txt`, `root/src/instructions.txt`, `root/src/Output/instructions.txt`

**Interfaces:**
- Produces a packaged WinUI application project named `WindowsAssetCreator`, a static library project named `AssetCore`, and a native test executable named `AssetCoreTests`.
- Produces the shared result vocabulary in `AssetTypes.h`, used by every later core and test task.
- Produces a minimal native test harness that can prove the test executable itself works without depending on Task 2 types.

- [ ] **Step 1: Create the MSBuild solution and three project boundaries**

Create the packaged WinUI 3/C++/WinRT application directly under `root/windows`, targeting C++20. The project structure must be compatible with the C++ “Blank App, Packaged (WinUI 3 in Desktop)” template, but do not require the Visual Studio IDE to edit or build it. Set a project reference from `WindowsAssetCreator.vcxproj` to `..\src\AssetCore.vcxproj`.

Create the `AssetCore` static-library project under `root/src` and the `AssetCoreTests` native console test project under `root/tests`, with a project reference to `..\src\AssetCore.vcxproj`.

The application project must be packaged (single-project MSIX) and use Windows App SDK/WinUI 3 dependencies. Do not add a C# or .NET project.

- [ ] **Step 2: Add the common core types in `root/src/AssetTypes.h`**

```cpp
#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace wac {
enum class Severity { info, warning, error };
enum class DiagnosticCode { unsupported_image, corrupt_image, wic_failure, io_failure,
                            validation_failure, export_cancelled, zip_failure };
struct Diagnostic { Severity severity; DiagnosticCode code; std::wstring message; std::wstring detail; };
struct PixelSize { uint32_t width; uint32_t height; };
enum class AssetFormat { png, ico };
struct AssetSpec { std::wstring group; std::filesystem::path relative_path; PixelSize size; AssetFormat format; };
struct RgbaPixel { uint8_t r; uint8_t g; uint8_t b; uint8_t a; };
struct OperationResult { std::vector<Diagnostic> diagnostics; bool succeeded() const noexcept; };
template <typename T>
struct GenerationResult { std::optional<T> value; std::vector<Diagnostic> diagnostics; bool succeeded() const noexcept; };
struct GeneratedAsset { AssetSpec spec; std::filesystem::path staged_path; };
}
```

- [ ] **Step 3: Implement the smallest native test harness in `root/tests/NativeTest.*`**

```cpp
// NativeTest.h
#define TEST_CASE(name) void name(); static wac_test::Registrar reg_##name{#name, name}; void name()
#define REQUIRE_EQ(actual, expected) wac_test::RequireEqual((actual), (expected), #actual, #expected, __FILE__, __LINE__)
int main();
```

The executable must run every registered test, print one pass/fail line per test, and return nonzero when any assertion fails. Keep all harness code under `root/tests`.

Add one trivial harness test that is expected to pass. Do **not** reference `StoreMsixProfile` in Task 1; that interface belongs to Task 2.

- [ ] **Step 4: Replace the four placeholder instruction files with short component-boundary notes**

Write only the responsibilities established in the Planned File Structure. Do not add product documentation or implementation details outside their owning directory.

- [ ] **Step 5: Build the complete Task 1 solution**

Run:

```powershell
msbuild root/WindowsAssetCreator.sln /t:Build /p:Configuration=Debug /p:Platform=x64
```

Expected:

- `WindowsAssetCreator` builds.
- `AssetCore.lib` builds.
- `AssetCoreTests.exe` builds.
- No intentional missing-interface compilation failure remains in Task 1.

If `msbuild` or a required WinUI/Windows SDK workload is missing, report that prerequisite once and stop verification there. Do not substitute self-review for a compiler result.

- [ ] **Step 6: Run the native harness**

Run:

```powershell
root/tests/x64/Debug/AssetCoreTests.exe
```

Expected: the harness test passes and the process exits successfully.

- [ ] **Task 1 gate: report and stop**

Report only the files changed, build result, test result, and any genuine blocker. Do not create a task report file. Do not commit automatically. **Stop and wait for external review before Task 2.**

## Task 2: Implement the internal Store/MSIX asset profile

**Files:**
- Create: `root/src/StoreMsixProfile.h`, `root/src/StoreMsixProfile.cpp`, `root/tests/ProfileTests.cpp`
- Modify: `root/src/AssetTypes.h`, `root/src/AssetCore.vcxproj`, `root/tests/AssetCoreTests.vcxproj`
- Test: `root/tests/ProfileTests.cpp`

**Interfaces:**
- Consumes: `wac::AssetSpec`, `wac::PixelSize`, and `wac::AssetFormat` from `AssetTypes.h`.
- Produces: `wac::StoreMsixProfile::Create()`, `png_assets()`, `ico_asset()`, and `all_relative_paths()`.

- [ ] **Step 1: Add filename, path, count, and dimension tests**

```cpp
TEST_CASE(Store_profile_has_all_required_groups)
{
    const auto profile = wac::StoreMsixProfile::Create();
    REQUIRE(profile.contains(L"Assets/AppList.targetsize-16.png"));
    REQUIRE(profile.contains(L"Assets/AppList.targetsize-256_altform-unplated.png"));
    REQUIRE(profile.contains(L"Assets/AppList.targetsize-256_altform-lightunplated.png"));
    REQUIRE(profile.contains(L"Assets/Square44x44Logo.scale-400.png"));
    REQUIRE(profile.contains(L"Assets/Square150x150Logo.scale-400.png"));
    REQUIRE(profile.contains(L"Assets/StoreLogo.scale-400.png"));
    REQUIRE(profile.contains(L"Assets/MedTile.scale-400.png"));
}

TEST_CASE(Store_profile_maps_scale_125_to_rounded_dimensions)
{
    const auto asset = wac::StoreMsixProfile::Create().find(L"Assets/StoreLogo.scale-125.png");
    REQUIRE_EQ(asset->size.width, uint32_t{63});
    REQUIRE_EQ(asset->size.height, uint32_t{63});
}
```


- [ ] **Step 2: Define the profile API and implement the declarative asset table**

```cpp
class StoreMsixProfile final {
public:
    static StoreMsixProfile Create();
    std::span<AssetSpec const> png_assets() const noexcept;
    AssetSpec const& ico_asset() const noexcept;
    AssetSpec const* find(std::filesystem::path const& relative_path) const noexcept;
    bool contains(std::filesystem::path const& relative_path) const noexcept;
private:
    std::vector<AssetSpec> png_assets_;
    AssetSpec ico_asset_;
};
```

Populate exactly these groups: 14 default AppList target sizes, 14 `_altform-unplated`, 14 `_altform-lightunplated`, Square44 base plus seven scales, Square150 base plus seven scales, StoreLogo base plus five scales, and five MedTile scales. Set every PNG path below `Assets/`; set the only ICO path to `AppIcon.ico`.

- [ ] **Step 3: Add profile integrity checks that reject duplicate paths, zero dimensions, non-PNG planned PNG assets, and a count other than 69**

```cpp
OperationResult ValidateProfile(StoreMsixProfile const& profile);
```

Return a `validation_failure` diagnostic naming the first bad profile entry. Invoke this validation before every generation session begins.

- [ ] **Step 4: Run the full profile test executable**

Run: `root/tests/x64/Debug/AssetCoreTests.exe`

Expected: all profile tests pass and report 69 PNG assets plus `AppIcon.ico`.

- [ ] **Task 2 gate: report and stop**

Report only the files changed, build result, test result, and any genuine unresolved concern. Do not create a task report file. Do not commit automatically. **Stop and wait for external review before continuing.**

## Task 3: Decode, orient, normalize, and resize images with WIC

**Files:**
- Create: `root/src/ImagePipeline.h`, `root/src/ImagePipeline.cpp`
- Create: `root/tests/TestImageFactory.h`, `root/tests/TestImageFactory.cpp`, `root/tests/ImagePipelineTests.cpp`
- Modify: `root/src/AssetCore.vcxproj`, `root/tests/AssetCoreTests.vcxproj`

**Interfaces:**
- Consumes: a source `std::filesystem::path` and `PixelSize`.
- Produces: `DecodedImage`, `NormalizeToSquare`, and `ResizeRgba` for Task 4's encoders.

- [ ] **Step 1: Add WIC pipeline tests using deterministic PNG and JPEG fixture files below `root/tests/TestImages`**

```cpp
TEST_CASE(Non_square_source_is_centered_without_crop)
{
    auto image = wac::LoadImage(TestImage(L"wide-red-blue.png"));
    auto normalized = wac::NormalizeToSquare(image);
    REQUIRE_EQ(normalized.size().width, uint32_t{400});
    REQUIRE_EQ(normalized.size().height, uint32_t{400});
    REQUIRE_EQ(normalized.pixel_at(0, 0).a, uint8_t{0});
    REQUIRE_EQ(normalized.pixel_at(20, 200).r, uint8_t{255});
    REQUIRE_EQ(normalized.pixel_at(379, 200).b, uint8_t{255});
}

TEST_CASE(Corrupt_source_returns_corrupt_image_diagnostic)
{
    const auto result = wac::LoadImage(TestImage(L"corrupt.png"));
    REQUIRE_EQ(result.diagnostics.front().code, wac::DiagnosticCode::corrupt_image);
}
```


- [ ] **Step 2: Implement the WIC decode and RGBA conversion boundary**

```cpp
class DecodedImage final {
public:
    PixelSize size() const noexcept;
    RgbaPixel pixel_at(uint32_t x, uint32_t y) const;
    winrt::com_ptr<IWICBitmap> bitmap() const noexcept;
};
GenerationResult<DecodedImage> LoadImage(std::filesystem::path const& source);
```

Create the WIC imaging factory once per operation, decode from the source file, apply EXIF orientation through WIC, convert to `GUID_WICPixelFormat32bppPBGRA` or a documented 32-bit RGBA equivalent, and map decoder failures to `unsupported_image` or `corrupt_image` diagnostics. Accept WIC-supported PNG and JPEG input in the MVP.

- [ ] **Step 3: Implement transparent-square normalization and high-quality resize**

```cpp
DecodedImage NormalizeToSquare(DecodedImage const& source);
DecodedImage ResizeRgba(DecodedImage const& source, PixelSize target);
```

Allocate a transparent 32-bit WIC bitmap with side `max(width, height)`, center the unscaled source on it, and copy pixels without cropping. Use `IWICBitmapScaler` with `WICBitmapInterpolationModeFant` for every output resize. Preserve alpha in the resulting bitmap.

- [ ] **Step 4: Add square-input, JPEG, EXIF-orientation, alpha, and resize-dimension tests**

```cpp
TEST_CASE(Square_source_is_not_reframed);
TEST_CASE(Jpeg_source_converts_to_opaque_rgba);
TEST_CASE(Exif_rotated_source_has_correct_display_orientation);
TEST_CASE(Resize_returns_exact_target_dimensions);
```

Generate fixtures through `TestImageFactory` so the test suite does not depend on external files or a Python runtime.

- [ ] **Step 5: Run all image and profile tests**

Run: `root/tests/x64/Debug/AssetCoreTests.exe`

Expected: all tests pass, including explicit transparent padding and no-crop assertions.

- [ ] **Task 3 gate: report and stop**

Report only the files changed, build result, test result, and any genuine unresolved concern. Do not create a task report file. Do not commit automatically. **Stop and wait for external review before continuing.**

## Task 4: Encode PNG assets, write ICO, and validate staged image files

**Files:**
- Create: `root/src/Output/PngEncoder.h`, `root/src/Output/PngEncoder.cpp`
- Create: `root/src/Output/IcoWriter.h`, `root/src/Output/IcoWriter.cpp`
- Create: `root/src/Output/Validator.h`, `root/src/Output/Validator.cpp`
- Create: `root/tests/OutputTests.cpp`
- Modify: `root/src/AssetCore.vcxproj`, `root/tests/AssetCoreTests.vcxproj`

**Interfaces:**
- Consumes: `DecodedImage`, `AssetSpec`, and the Store profile from Tasks 2–3.
- Produces: WIC-encoded PNGs, `AppIcon.ico`, and `ValidateStagedAssets` for Task 5.

- [ ] **Step 1: Add output tests for a representative PNG and the ICO directory**

```cpp
TEST_CASE(Png_encoder_writes_exact_rgba_dimensions)
{
    const auto output = TestTempPath(L"square44.png");
    REQUIRE(wac::EncodePng(TestImage(), output, {44, 44}).succeeded());
    REQUIRE_EQ(wac::LoadImage(output).value->size(), wac::PixelSize{44, 44});
}

TEST_CASE(Ico_writer_contains_five_requested_sizes)
{
    REQUIRE(wac::WriteAppIcon(TestImage(), TestTempPath(L"AppIcon.ico")).succeeded());
    REQUIRE_EQ(wac::ReadIcoDirectory(TestTempPath(L"AppIcon.ico")), std::vector<uint32_t>({16, 24, 32, 48, 256}));
}
```


- [ ] **Step 2: Implement WIC PNG encoding**

```cpp
OperationResult EncodePng(DecodedImage const& normalized_source,
                          std::filesystem::path const& destination,
                          PixelSize target_size);
```

Resize through `ResizeRgba`, create an `IWICStream`, `IWICBitmapEncoder` with `GUID_ContainerFormatPng`, and one frame. Write the frame atomically to a staging path. Return `io_failure` or `wic_failure` diagnostics with the destination path when any WIC call fails.

- [ ] **Step 3: Implement the small native ICO writer**

```cpp
OperationResult WriteAppIcon(DecodedImage const& normalized_source,
                             std::filesystem::path const& destination);
```

Encode five PNG payloads at 16, 24, 32, 48, and 256 pixels. Write an ICO header, five directory entries, and the payloads. Use `0` in the ICO directory width/height byte for the 256-pixel entry. Validate offsets, byte counts, and PNG signatures before closing the file.

- [ ] **Step 4: Implement staged-file validation**

```cpp
OperationResult ValidateStagedAssets(StoreMsixProfile const& profile,
                                     std::filesystem::path const& staging_root);
```

For each profile PNG, confirm the file exists, re-open it with WIC, verify exact dimensions, and verify the decoded pixel format is alpha-capable. Parse the ICO header/directory and require the five exact sizes. Reject an unexpected missing or duplicate package path.

- [ ] **Step 5: Add all-profile validation and malformed-file tests**

```cpp
TEST_CASE(Validator_accepts_complete_profile_output);
TEST_CASE(Validator_rejects_wrong_png_dimensions);
TEST_CASE(Validator_rejects_missing_asset);
TEST_CASE(Validator_rejects_malformed_ico);
```

- [ ] **Step 6: Run output, image, and profile tests**

Run: `root/tests/x64/Debug/AssetCoreTests.exe`

Expected: every test passes and WIC can re-decode every test PNG.

- [ ] **Task 4 gate: report and stop**

Report only the files changed, build result, test result, and any genuine unresolved concern. Do not create a task report file. Do not commit automatically. **Stop and wait for external review before continuing.**

## Task 5: Create temporary generation sessions and the ZIP exporter

**Files:**
- Create: `root/src/Output/StagingSession.h`, `root/src/Output/StagingSession.cpp`
- Create: `root/src/Output/ZipWriter.h`, `root/src/Output/ZipWriter.cpp`
- Create: `root/src/AssetGenerator.h`, `root/src/AssetGenerator.cpp`
- Create: `root/tests/GenerationSessionTests.cpp`
- Modify: `root/src/AssetCore.vcxproj`, `root/tests/AssetCoreTests.vcxproj`

**Interfaces:**
- Consumes: the fixed profile, normalized image pipeline, encoders, and validator from Tasks 2–4.
- Produces: `GenerationSession`, `AssetGenerator::Generate`, and `ExportZip` for the WinUI app.

- [ ] **Step 1: Add generation and export tests**

```cpp
TEST_CASE(Generation_creates_valid_69_png_and_ico_session)
{
    auto result = wac::AssetGenerator{}.Generate(TestImage(L"wide-red-blue.png"));
    REQUIRE(result.succeeded());
    const auto& session = *result.value;
    REQUIRE_EQ(session.preview_assets().size(), size_t{70});
    REQUIRE(wac::ValidateStagedAssets(session.profile(), session.staging_root()).succeeded());
}

TEST_CASE(Export_zip_contains_only_assets_and_appicon)
{
    auto session = GenerateReadySession();
    const auto zip = TestTempPath(L"Windows-Assets.zip");
    REQUIRE(session.ExportZip(zip).succeeded());
    REQUIRE_EQ(wac::ReadZipEntries(zip).size(), size_t{70});
    REQUIRE(wac::ZipContains(zip, L"Assets/AppList.targetsize-16.png"));
    REQUIRE(wac::ZipContains(zip, L"AppIcon.ico"));
}
```


- [ ] **Step 2: Implement owned temporary staging sessions**

```cpp
class GenerationSession final {
public:
    std::filesystem::path const& staging_root() const noexcept;
    StoreMsixProfile const& profile() const noexcept;
    std::span<GeneratedAsset const> preview_assets() const noexcept;
    OperationResult ExportZip(std::filesystem::path const& user_destination) const;
};
```

Create a unique directory below the process temporary path. `GenerationSession` owns it and deletes only that exact directory in its destructor. `AssetGenerator::Generate` validates the profile, loads/normalizes the source once, writes every profile asset into this staging root, writes `AppIcon.ico`, validates all staged files, and returns thumbnail paths plus structured diagnostics.

```cpp
class AssetGenerator final {
public:
    GenerationResult<GenerationSession> Generate(std::filesystem::path const& source) const;
};
```

- [ ] **Step 3: Implement the ZIP writer with standard stored ZIP entries**

```cpp
OperationResult WriteZip(std::filesystem::path const& source_root,
                         std::span<std::filesystem::path const> relative_entries,
                         std::filesystem::path const& destination);
```

Use a small self-contained ZIP writer: calculate CRC-32, emit local headers, central-directory headers, and end-of-central-directory records. Store PNG/ICO entries without recompressing them because the payloads are already compressed. Write to `<destination>.tmp`, then replace the chosen destination only after the complete archive is closed and validated. The ZIP entry list must come from the profile, not directory enumeration.

- [ ] **Step 4: Implement ZIP inspection helpers only inside `root/tests`**

```cpp
std::vector<std::filesystem::path> ReadZipEntries(std::filesystem::path const& archive);
bool ZipContains(std::filesystem::path const& archive, std::filesystem::path const& entry);
```

Use them to assert no master source image, no temporary files, and no product-specific `HeadsUp` name enter the archive.

- [ ] **Step 5: Add error and cleanup tests**

```cpp
TEST_CASE(Corrupt_source_does_not_create_ready_session);
TEST_CASE(Export_failure_keeps_existing_user_zip_unchanged);
TEST_CASE(Session_cleanup_removes_only_its_own_staging_directory);
TEST_CASE(Zip_manifest_rejects_missing_planned_file);
```

- [ ] **Step 6: Run the complete native test executable**

Run: `root/tests/x64/Debug/AssetCoreTests.exe`

Expected: all generation, ZIP, image, profile, ICO, and validation tests pass.

- [ ] **Task 5 gate: report and stop**

Report only the files changed, build result, test result, and any genuine unresolved concern. Do not create a task report file. Do not commit automatically. **Stop and wait for external review before continuing.**

## Task 6: Build the approved WinUI asset-board shell

**Files:**
- Create: `root/windows/AssetBoardState.h`, `root/windows/AssetBoardState.cpp`
- Create: `root/windows/AssetBoardViewModel.h`, `root/windows/AssetBoardViewModel.cpp`
- Modify: `root/windows/MainWindow.xaml`, `root/windows/MainWindow.xaml.h`, `root/windows/MainWindow.xaml.cpp`, `root/windows/WindowsAssetCreator.vcxproj`, `root/tests/AssetCoreTests.vcxproj`
- Test: `root/tests/GenerationSessionTests.cpp`

**Interfaces:**
- Consumes: `wac::AssetGenerator`, `wac::GenerationSession`, `wac::Diagnostic`, and generated preview paths from Task 5.
- Produces: a WinUI-facing `AssetBoardViewModel` with `Idle`, `Processing`, `Ready`, `Saving`, and `Error` states.

- [ ] **Step 1: Add state-transition tests to the native test project**

```cpp
TEST_CASE(Board_state_is_ready_only_after_valid_generation)
{
    wac::AssetBoardState state;
    state.BeginGeneration();
    state.CompleteGeneration(GenerateReadySession());
    REQUIRE_EQ(state.phase(), wac::BoardPhase::ready);
    REQUIRE(state.can_save());
}

TEST_CASE(Board_state_keeps_save_disabled_after_corrupt_source)
{
    wac::AssetBoardState state;
    state.CompleteFailure(CorruptImageDiagnostic());
    REQUIRE_EQ(state.phase(), wac::BoardPhase::error);
    REQUIRE(!state.can_save());
}
```


- [ ] **Step 2: Implement UI-independent board state and the WinUI view model**

```cpp
enum class BoardPhase { idle, processing, ready, saving, error };
class AssetBoardState final {
public:
    void BeginGeneration();
    void CompleteGeneration(GenerationSession session);
    void CompleteFailure(std::vector<Diagnostic> diagnostics);
    BoardPhase phase() const noexcept;
    bool can_save() const noexcept;
};
```

Keep `AssetBoardState` in `root/windows` but free of WinUI types so `root/tests` can compile its `.cpp` file directly and test it without a XAML host. `AssetBoardViewModel` adapts it to observable XAML properties: source name, source dimensions, source framing note, grouped generated assets, validation text, busy state, error text, and `CanSave`.

- [ ] **Step 3: Implement the approved XAML board layout**

Use a `Grid` with a compact source/drop panel at the top, a `ScrollViewer` containing grouped `ItemsRepeater`/`GridView` thumbnail sections, an `InfoBar` for diagnostics, and a bottom command area holding `Reset` and `Save As…`. Show the empty drop target only in `idle`; retain the source summary and validation result in `ready`.

```xml
<ScrollViewer Grid.Row="1">
  <ItemsRepeater ItemsSource="{x:Bind ViewModel.Groups}">
    <ItemsRepeater.ItemTemplate>
      <DataTemplate x:DataType="local:AssetGroupViewModel">
        <StackPanel Spacing="8">
          <TextBlock Text="{x:Bind Title}" Style="{ThemeResource SubtitleTextBlockStyle}" />
          <ItemsRepeater ItemsSource="{x:Bind Assets}" />
        </StackPanel>
      </DataTemplate>
    </ItemsRepeater.ItemTemplate>
  </ItemsRepeater>
</ScrollViewer>
```

- [ ] **Step 4: Build and manually smoke-test the shell**

Run: `msbuild root/WindowsAssetCreator.sln /t:Build /p:Configuration=Debug /p:Platform=x64`

Expected: the packaged app opens with an accessible empty drop target, disabled Save As button, and no generator work on the UI thread.

- [ ] **Step 5: Run the native state and core tests**

Run: `root/tests/x64/Debug/AssetCoreTests.exe`

Expected: board state tests and all core tests pass.

- [ ] **Task 6 gate: report and stop**

Report only the files changed, build result, test result, and any genuine unresolved concern. Do not create a task report file. Do not commit automatically. **Stop and wait for external review before continuing.**

## Task 7: Connect image intake, Save As, and Explorer reveal

**Files:**
- Create: `root/windows/ExplorerReveal.h`, `root/windows/ExplorerReveal.cpp`
- Modify: `root/windows/MainWindow.xaml`, `root/windows/MainWindow.xaml.h`, `root/windows/MainWindow.xaml.cpp`, `root/windows/AssetBoardViewModel.*`
- Test: `root/tests/GenerationSessionTests.cpp`

**Interfaces:**
- Consumes: the board view model and `GenerationSession::ExportZip` from Tasks 5–6.
- Produces: working drag/drop, browse, asynchronous generation, Save As, and Explorer-reveal behavior.

- [ ] **Step 1: Add command-state tests for Save As cancellation and export failure**

```cpp
TEST_CASE(Cancelled_save_returns_board_to_ready_state)
{
    auto state = ReadyBoardState();
    state.BeginSave();
    state.CompleteSaveCancelled();
    REQUIRE_EQ(state.phase(), wac::BoardPhase::ready);
    REQUIRE(state.can_save());
}

TEST_CASE(Failed_export_exposes_diagnostic_and_preserves_ready_session)
{
    auto state = ReadyBoardState();
    state.CompleteSaveFailure(ZipFailureDiagnostic());
    REQUIRE_EQ(state.phase(), wac::BoardPhase::ready);
    REQUIRE(state.can_save());
    REQUIRE(!state.diagnostics().empty());
}
```


- [ ] **Step 2: Implement browse and drag/drop intake**

Attach a `FileOpenPicker` to Browse, initialize it with the WinUI window HWND through `IInitializeWithWindow`, restrict its visible choices to `.png`, `.jpg`, and `.jpeg`, and accept exactly one file. Enable `AllowDrop` on the main drop surface, inspect `DataPackageView` for one storage file, and reject folders/multiple files with an `unsupported_image` diagnostic. Dispatch `AssetGenerator::Generate` on a background thread; update `AssetBoardViewModel` only through the UI dispatcher.

- [ ] **Step 3: Extend the tested board state for save transitions**

```cpp
void AssetBoardState::BeginSave();
void AssetBoardState::CompleteSaveCancelled();
void AssetBoardState::CompleteSaveFailure(Diagnostic diagnostic);
```

`BeginSave` changes `ready` to `saving`. `CompleteSaveCancelled` restores `ready` without adding an error. `CompleteSaveFailure` restores `ready`, retains the current `GenerationSession`, appends the diagnostic, and leaves `can_save()` true so the user can choose a different ZIP destination.

- [ ] **Step 4: Implement Save As as the sole user-output choice**

Use a Windows `FileSavePicker` initialized with the WinUI window HWND through `IInitializeWithWindow`, configured for ZIP files, and suggest `Windows-Assets.zip`. Do nothing if the user cancels. On selection, call `GenerationSession::ExportZip` off the UI thread. Move the board to `saving` while exporting; on success return to `ready` with a saved confirmation, and on failure return to `ready` with the diagnostic while retaining the preview session so the user can retry Save As.

- [ ] **Step 5: Implement Explorer reveal after successful export**

```cpp
void RevealInExplorer(std::filesystem::path const& saved_zip)
{
    const std::wstring arguments = L"/select,\"" + saved_zip.wstring() + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", arguments.c_str(), nullptr, SW_SHOWNORMAL);
}
```

Treat a failed `ShellExecuteW` result as a nonfatal warning: the ZIP is already saved, so keep the success state and show a “Saved, but Explorer could not be opened” message.

- [ ] **Step 6: Run native tests and perform the complete WinUI smoke matrix**

Run: `root/tests/x64/Debug/AssetCoreTests.exe`

Expected: all state-transition and core tests pass.

Manually verify in the packaged app:

1. Drop a PNG and verify the scrollable board shows all groups and `69 PNG + 1 ICO ready`.
2. Browse to a JPEG and verify preview/generation succeeds.
3. Drop a corrupt/unsupported file and verify a clear error with Save As disabled.
4. Select Save As then cancel; verify the board remains ready and no ZIP appears.
5. Save a ZIP; verify its contents, then verify Explorer opens with it selected.
6. Generate a second source; verify the old staging session is removed and the new preview replaces it.

- [ ] **Step 7: Update the README with build prerequisites and the three-step user workflow**

Document the command-line build prerequisites: Visual Studio Build Tools/MSBuild with the native C++ and WinUI application build workloads, the Windows SDK, and Windows App SDK dependencies required by the project. Visual Studio IDE may be mentioned as optional, not required. Document building the solution, running `AssetCoreTests`, and the drag/drop → review → Save As flow. Do not document telemetry, accounts, or a CLI because none exist.

- [ ] **Task 7 gate: report and stop**

Report only the files changed, build result, test result, and any genuine unresolved concern. Do not create a task report file. Do not commit automatically. **Stop and wait for external review before continuing.**

## Plan Review Checklist

- [x] Every design requirement maps to one or more tasks: native WinUI, isolated core, WIC processing, no crop, exact 69+ICO profile, validation, staging, ZIP, Save As, Explorer reveal, and tests under `root/tests`.
- [x] No UI code is planned below `root/src`, and no test code is planned outside `root/tests`.
- [x] All later interfaces are introduced by an earlier task with named functions and types.
- [x] The profile and ZIP are manifest-driven, preventing accidental product-specific or extra files.
- [x] Placeholder review completed: no unresolved work markers or deferred implementation steps remain.
- [x] Execution is single-agent by default; subagents require explicit user authorization.
- [x] Each task ends at an external-review gate; later tasks may not begin early.
- [x] No implementation-report/evidence Markdown or automatic commits are required.
- [x] Expected intermediate compilation failures are not used as ceremonial gates; each task aims to finish buildable and testable.
