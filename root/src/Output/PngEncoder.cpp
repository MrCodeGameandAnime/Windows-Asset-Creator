#include "PngEncoder.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <filesystem>
#include <stdexcept>
#include <string>

namespace wac {
namespace {
using Microsoft::WRL::ComPtr;
class Apartment { public: Apartment() : result_(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) {} ~Apartment() { if (result_ == S_OK || result_ == S_FALSE) CoUninitialize(); } HRESULT result_; };
class HResultError final : public std::runtime_error {
public:
    explicit HResultError(HRESULT result) : std::runtime_error("WIC failure"), result(result) {}
    HRESULT result;
};
void Check(HRESULT result) { if (FAILED(result)) throw HResultError(result); }
OperationResult Failure(DiagnosticCode code, std::filesystem::path const& path, std::wstring detail = {}) {
    return {{{Severity::error, code, L"Unable to encode PNG asset.", detail.empty() ? path.wstring() : std::move(detail)}}};
}
}

OperationResult EncodePng(DecodedImage const& normalized_source, std::filesystem::path const& destination,
                          PixelSize target_size) {
    try {
        Apartment apartment;
        Check(apartment.result_);
        std::filesystem::create_directories(destination.parent_path());
        const auto image = ResizeRgba(normalized_source, target_size);
        ComPtr<IWICImagingFactory> factory;
        Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf())));
        ComPtr<IWICStream> stream;
        ComPtr<IWICBitmapEncoder> encoder;
        ComPtr<IWICBitmapFrameEncode> frame;
        ComPtr<IPropertyBag2> options;
        const auto temporary = destination.wstring() + L".tmp";
        Check(factory->CreateStream(stream.GetAddressOf()));
        Check(stream->InitializeFromFilename(temporary.c_str(), GENERIC_WRITE));
        Check(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.GetAddressOf()));
        Check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache));
        Check(encoder->CreateNewFrame(frame.GetAddressOf(), options.GetAddressOf()));
        Check(frame->Initialize(options.Get()));
        Check(frame->SetSize(target_size.width, target_size.height));
        GUID format = GUID_WICPixelFormat32bppRGBA;
        Check(frame->SetPixelFormat(&format));
        Check(frame->WriteSource(image.bitmap().Get(), nullptr));
        Check(frame->Commit());
        Check(encoder->Commit());
        frame.Reset();
        options.Reset();
        encoder.Reset();
        stream.Reset();
        std::filesystem::rename(temporary, destination);
        return {};
    } catch (std::filesystem::filesystem_error const&) {
        return Failure(DiagnosticCode::io_failure, destination);
    } catch (HResultError const& error) {
        return Failure(DiagnosticCode::wic_failure, destination,
                       destination.wstring() + L" (WIC HRESULT " + std::to_wstring(static_cast<uint32_t>(error.result)) + L")");
    } catch (...) {
        return Failure(DiagnosticCode::wic_failure, destination);
    }
}
}
