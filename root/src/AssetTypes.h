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
struct OperationResult {
    std::vector<Diagnostic> diagnostics;
    bool succeeded() const noexcept {
        for (const auto& diagnostic : diagnostics) {
            if (diagnostic.severity == Severity::error) return false;
        }
        return true;
    }
};
template <typename T>
struct GenerationResult {
    std::optional<T> value;
    std::vector<Diagnostic> diagnostics;
    bool succeeded() const noexcept {
        if (!value) return false;
        for (const auto& diagnostic : diagnostics) {
            if (diagnostic.severity == Severity::error) return false;
        }
        return true;
    }
};
struct GeneratedAsset { AssetSpec spec; std::filesystem::path staged_path; };
}
