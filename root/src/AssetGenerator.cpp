#include "AssetGenerator.h"
#include "ImagePipeline.h"
#include "Output/IcoWriter.h"
#include "Output/PngEncoder.h"
#include "Output/Validator.h"

#include <chrono>

namespace wac {
namespace {
GenerationResult<GenerationSession> Failure(std::vector<Diagnostic> diagnostics) { return {std::nullopt, std::move(diagnostics)}; }
}
GenerationResult<GenerationSession> AssetGenerator::Generate(std::filesystem::path const& source) const {
    auto profile = StoreMsixProfile::Create();
    const auto profile_result = ValidateProfile(profile);
    if (!profile_result.succeeded()) return Failure(profile_result.diagnostics);
    const auto decoded = LoadImage(source);
    if (!decoded.succeeded()) return Failure(decoded.diagnostics);
    const auto normalized = NormalizeToSquare(*decoded.value);
    const auto root = std::filesystem::temp_directory_path() / L"WindowsAssetCreator" /
                      std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count());
    try { std::filesystem::create_directories(root); }
    catch (...) { return Failure({{Severity::error, DiagnosticCode::io_failure, L"Unable to create staging directory.", root.wstring()}}); }
    std::vector<GeneratedAsset> assets;
    for (const auto& spec : profile.png_assets()) {
        const auto output = root / spec.relative_path;
        const auto result = EncodePng(normalized, output, spec.size);
        if (!result.succeeded()) { std::filesystem::remove_all(root); return Failure(result.diagnostics); }
        assets.push_back({spec, output});
    }
    const auto ico = profile.ico_asset();
    const auto ico_result = WriteAppIcon(normalized, root / ico.relative_path);
    if (!ico_result.succeeded()) { std::filesystem::remove_all(root); return Failure(ico_result.diagnostics); }
    assets.push_back({ico, root / ico.relative_path});
    const auto validation = ValidateStagedAssets(profile, root);
    if (!validation.succeeded()) { std::filesystem::remove_all(root); return Failure(validation.diagnostics); }
    return {GenerationSession{root, std::move(profile), std::move(assets)}, {}};
}
}
