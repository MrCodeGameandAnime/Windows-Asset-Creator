#include "AssetGenerator.h"
#include "Diagnostics/TraceSink.h"
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
        trace_sink::Emit(L"STAGING", L"staging-root creation BEGIN parent=" + parent.wstring());
        std::error_code error;
        std::filesystem::create_directories(parent, error);
        if (error) {
            trace_sink::Emit(L"STAGING", L"staging-root parent creation failed error=" + std::to_wstring(error.value()));
            return std::nullopt;
        }

        static std::atomic_uint64_t counter{0};
        for (uint32_t attempt = 0; attempt != 64; ++attempt) {
            const auto identifier = std::to_wstring(GetCurrentProcessId()) + L"-" +
                std::to_wstring(GetTickCount64()) + L"-" +
                std::to_wstring(counter.fetch_add(1, std::memory_order_relaxed));
            const auto root = parent / identifier;
            error.clear();
            if (std::filesystem::create_directory(root, error)) {
                trace_sink::Emit(L"STAGING", L"staging-root created path=" + root.wstring());
                return root;
            }
            if (error) {
                trace_sink::Emit(L"STAGING", L"staging-root creation failed error=" + std::to_wstring(error.value()));
                return std::nullopt;
            }
        }
    } catch (...) {
        trace_sink::Emit(L"STAGING", L"staging-root creation threw");
        return std::nullopt;
    }
    trace_sink::Emit(L"STAGING", L"staging-root creation exhausted attempts");
    return std::nullopt;
}
void Cleanup(std::filesystem::path const& root) noexcept {
    std::error_code error;
    std::filesystem::remove_all(root, error);
}
}
GenerationResult<GenerationSession> AssetGenerator::Generate(std::filesystem::path const& source) const {
    std::optional<trace_sink::OperationTagScope> generated_tag;
    if (!trace_sink::HasOperationTag()) {
        generated_tag.emplace(L"GEN#" + std::to_wstring(trace_sink::NextOperationId()));
    }
    trace_sink::Emit(L"GENERATE", L"AssetGenerator::Generate ENTER source=" + source.wstring());

    auto profile = StoreMsixProfile::Create();
    trace_sink::Emit(L"GENERATE", L"profile validation BEGIN");
    const auto profile_result = ValidateProfile(profile);
    if (!profile_result.succeeded()) {
        trace_sink::Emit(L"GENERATE", L"profile validation result=failure");
        trace_sink::Emit(L"GENERATE", L"AssetGenerator::Generate EXIT failure");
        return Failure(profile_result.diagnostics);
    }
    trace_sink::Emit(L"GENERATE", L"profile validation result=success png_count=" +
                              std::to_wstring(profile.png_assets().size()) + L" ico=1");

    trace_sink::Emit(L"WIC", L"LoadImage ENTER path=" + source.wstring());
    const auto decoded = LoadImage(source);
    if (!decoded.succeeded()) {
        trace_sink::Emit(L"GENERATE", L"AssetGenerator::Generate EXIT failure at LoadImage");
        return Failure(decoded.diagnostics);
    }
    trace_sink::Emit(L"WIC", L"LoadImage result=success dimensions=" + std::to_wstring(decoded.value->size().width) +
                              L"x" + std::to_wstring(decoded.value->size().height));

    trace_sink::Emit(L"WIC", L"normalization BEGIN");
    const auto normalized = NormalizeToSquare(*decoded.value);
    trace_sink::Emit(L"WIC", L"normalization result=" + std::to_wstring(normalized.size().width) +
                              L"x" + std::to_wstring(normalized.size().height));
    const auto root = CreateStagingRoot();
    if (!root) {
        trace_sink::Emit(L"GENERATE", L"AssetGenerator::Generate EXIT failure at staging-root");
        return Failure({{Severity::error, DiagnosticCode::io_failure, L"Unable to create staging directory.", L"WindowsAssetCreator"}});
    }
    std::vector<GeneratedAsset> assets;
    size_t asset_index = 0;
    for (const auto& spec : profile.png_assets()) {
        const auto output = *root / spec.relative_path;
        if (asset_index == 0) trace_sink::Emit(L"GENERATE", L"first asset encode BEGIN path=" + spec.relative_path.wstring());
        const auto result = EncodePng(normalized, output, spec.size);
        if (!result.succeeded()) {
            trace_sink::Emit(L"GENERATE", L"asset encode FAILED path=" + spec.relative_path.wstring());
            Cleanup(*root);
            trace_sink::Emit(L"GENERATE", L"AssetGenerator::Generate EXIT failure at PNG encode");
            return Failure(result.diagnostics);
        }
        assets.push_back({spec, output});
        ++asset_index;
    }
    trace_sink::Emit(L"GENERATE", L"PNG output generation complete count=" + std::to_wstring(assets.size()));
    const auto ico = profile.ico_asset();
    const auto ico_result = WriteAppIcon(normalized, *root / ico.relative_path);
    if (!ico_result.succeeded()) {
        trace_sink::Emit(L"GENERATE", L"ICO generation result=failure path=" + ico.relative_path.wstring());
        Cleanup(*root);
        trace_sink::Emit(L"GENERATE", L"AssetGenerator::Generate EXIT failure at ICO");
        return Failure(ico_result.diagnostics);
    }
    assets.push_back({ico, *root / ico.relative_path});
    trace_sink::Emit(L"GENERATE", L"ICO generation result=success");
    trace_sink::Emit(L"VALIDATE", L"staged output validation BEGIN");
    const auto validation = ValidateStagedAssets(profile, *root);
    if (!validation.succeeded()) {
        trace_sink::Emit(L"VALIDATE", L"staged output validation result=failure");
        Cleanup(*root);
        trace_sink::Emit(L"GENERATE", L"AssetGenerator::Generate EXIT failure at validation");
        return Failure(validation.diagnostics);
    }
    trace_sink::Emit(L"VALIDATE", L"staged output validation result=success");
    trace_sink::Emit(L"GENERATE", L"AssetGenerator::Generate EXIT success assets=" + std::to_wstring(assets.size()));
    return {GenerationSession{*root, std::move(profile), std::move(assets)}, {}};
}
}
