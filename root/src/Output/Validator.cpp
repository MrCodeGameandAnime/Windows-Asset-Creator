#include "Validator.h"
#include "../ImagePipeline.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <unordered_set>

namespace wac {
namespace {
using Microsoft::WRL::ComPtr;
OperationResult Failure(std::filesystem::path const& path) {
    return {{{Severity::error, DiagnosticCode::validation_failure, L"Staged asset validation failed.", path.wstring()}}};
}
uint16_t ReadU16(std::istream& input) { uint8_t a = input.get(), b = input.get(); return static_cast<uint16_t>(a | (b << 8)); }
bool IsAlphaCapablePng(std::filesystem::path const& path) {
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf())))) return false;
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf()))) return false;
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, frame.GetAddressOf()))) return false;
    WICPixelFormatGUID format{};
    if (FAILED(frame->GetPixelFormat(&format))) return false;
    ComPtr<IWICComponentInfo> component;
    if (FAILED(factory->CreateComponentInfo(format, component.GetAddressOf()))) return false;
    ComPtr<IWICPixelFormatInfo2> pixel_format;
    if (FAILED(component.As(&pixel_format))) return false;
    BOOL supports_transparency = FALSE;
    return SUCCEEDED(pixel_format->SupportsTransparency(&supports_transparency)) && supports_transparency == TRUE;
}
}

OperationResult ValidateStagedAssets(StoreMsixProfile const& profile, std::filesystem::path const& staging_root) {
    if (!ValidateProfile(profile).succeeded()) return Failure(staging_root);
    std::unordered_set<std::wstring> paths;
    for (const auto& asset : profile.png_assets()) {
        if (!paths.insert(asset.relative_path.generic_wstring()).second) return Failure(asset.relative_path);
        const auto path = staging_root / asset.relative_path;
        if (!std::filesystem::exists(path)) return Failure(asset.relative_path);
        const auto image = LoadImage(path);
        if (!image.succeeded() || image.value->size().width != asset.size.width || image.value->size().height != asset.size.height ||
            !IsAlphaCapablePng(path)) return Failure(asset.relative_path);
    }
    const auto& ico = profile.ico_asset();
    if (!paths.insert(ico.relative_path.generic_wstring()).second) return Failure(ico.relative_path);
    const auto ico_path = staging_root / ico.relative_path;
    std::ifstream input(ico_path, std::ios::binary);
    if (!input || ReadU16(input) != 0 || ReadU16(input) != 1 || ReadU16(input) != 5) return Failure(ico.relative_path);
    constexpr std::array<uint32_t, 5> expected{16, 24, 32, 48, 256};
    for (const auto size : expected) {
        const auto width = static_cast<uint8_t>(input.get());
        const auto height = static_cast<uint8_t>(input.get());
        input.ignore(14);
        if ((size == 256 && (width != 0 || height != 0)) || (size != 256 && (width != size || height != size))) return Failure(ico.relative_path);
    }
    if (!input) return Failure(ico.relative_path);
    for (const auto& entry : std::filesystem::recursive_directory_iterator(staging_root)) {
        if (!entry.is_regular_file()) continue;
        const auto relative = std::filesystem::relative(entry.path(), staging_root).generic_wstring();
        if (!paths.contains(relative)) return Failure(entry.path());
    }
    return {};
}
}
