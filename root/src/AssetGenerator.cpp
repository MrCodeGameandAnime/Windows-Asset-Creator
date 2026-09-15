#include "AssetGenerator.h"
#include "ImagePipeline.h"
#include "Output/IcoWriter.h"
#include "Output/PngEncoder.h"
#include "Output/Validator.h"

#include <atomic>
#include <windows.h>

namespace wac {
namespace {
GenerationResult<GenerationSession> Failure(std::vector<Diagnostic> diagnostics) { return {std::nullopt, std::move(diagnostics)}; }
std::optional<std::filesystem::path> CreateStagingRoot() {
    try {
        const auto parent = std::filesystem::temp_directory_path() / L"WindowsAssetCreator";
        std::error_code error;
        std::filesystem::create_directories(parent, error);
        if (error) return std::nullopt;

        static std::atomic_uint64_t counter{0};
        for (uint32_t attempt = 0; attempt != 64; ++attempt) {
            const auto identifier = std::to_wstring(GetCurrentProcessId()) + L"-" +
                std::to_wstring(GetTickCount64()) + L"-" +
                std::to_wstring(counter.fetch_add(1, std::memory_order_relaxed));
            const auto root = parent / identifier;
            error.clear();
            if (std::filesystem::create_directory(root, error)) return root;
            if (error) return std::nullopt;
        }
    } catch (...) {
        return std::nullopt;
    }
    return std::nullopt;
}
void Cleanup(std::filesystem::path const& root) noexcept {
    std::error_code error;
    std::filesystem::remove_all(root, error);
}
}
GenerationResult<GenerationSession> AssetGenerator::Generate(std::filesystem::path const& source) const {
    auto profile = StoreMsixProfile::Create();
    const auto profile_result = ValidateProfile(profile);
    if (!profile_result.succeeded()) return Failure(profile_result.diagnostics);
    const auto decoded = LoadImage(source);
    if (!decoded.succeeded()) return Failure(decoded.diagnostics);
    const auto normalized = NormalizeToSquare(*decoded.value);
    const auto root = CreateStagingRoot();
    if (!root) return Failure({{Severity::error, DiagnosticCode::io_failure, L"Unable to create staging directory.", L"WindowsAssetCreator"}});
    std::vector<GeneratedAsset> assets;
    for (const auto& spec : profile.png_assets()) {
        const auto output = *root / spec.relative_path;
        const auto result = EncodePng(normalized, output, spec.size);
        if (!result.succeeded()) { Cleanup(*root); return Failure(result.diagnostics); }
        assets.push_back({spec, output});
    }
    const auto ico = profile.ico_asset();
    const auto ico_result = WriteAppIcon(normalized, *root / ico.relative_path);
    if (!ico_result.succeeded()) { Cleanup(*root); return Failure(ico_result.diagnostics); }
    assets.push_back({ico, *root / ico.relative_path});
    const auto validation = ValidateStagedAssets(profile, *root);
    if (!validation.succeeded()) { Cleanup(*root); return Failure(validation.diagnostics); }
    return {GenerationSession{*root, std::move(profile), std::move(assets)}, {}};
}
}
