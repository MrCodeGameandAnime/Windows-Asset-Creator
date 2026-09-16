#include "PngEncoder.h"
#include "../Diagnostics/TraceSink.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <filesystem>
#include <stdexcept>
#include <string>

namespace wac {
namespace {
using Microsoft::WRL::ComPtr;
class Apartment {
public:
    Apartment() : result_(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) {
        if (FAILED(result_)) trace_sink::EmitHr(L"WIC", L"EncodePng CoInitializeEx", static_cast<long>(result_));
    }
    ~Apartment() {
        if (result_ == S_OK || result_ == S_FALSE) CoUninitialize();
    }
    bool available() const noexcept {
        return SUCCEEDED(result_) || result_ == RPC_E_CHANGED_MODE;
    }
private:
    HRESULT result_;
};
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
        if (!apartment.available()) {
            trace_sink::Emit(L"WIC", L"EncodePng unavailable apartment");
            return Failure(DiagnosticCode::wic_failure, destination);
        }
        std::filesystem::create_directories(destination.parent_path());
        const auto image = ResizeRgba(normalized_source, target_size);
        ComPtr<IWICImagingFactory> factory;
        const auto factory_result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                                     IID_PPV_ARGS(factory.GetAddressOf()));
        if (FAILED(factory_result)) trace_sink::EmitHr(L"WIC", L"EncodePng CoCreateInstance(CLSID_WICImagingFactory)", static_cast<long>(factory_result));
        Check(factory_result);
        ComPtr<IWICStream> stream;
        ComPtr<IWICBitmapEncoder> encoder;
        ComPtr<IWICBitmapFrameEncode> frame;
        ComPtr<IPropertyBag2> options;
        const auto temporary = destination.wstring() + L".tmp";
        const auto stream_result = factory->CreateStream(stream.GetAddressOf());
        if (FAILED(stream_result)) trace_sink::EmitHr(L"WIC", L"IWICImagingFactory::CreateStream", static_cast<long>(stream_result));
        Check(stream_result);
        const auto stream_init_result = stream->InitializeFromFilename(temporary.c_str(), GENERIC_WRITE);
        if (FAILED(stream_init_result)) trace_sink::EmitHr(L"WIC", L"IWICStream::InitializeFromFilename", static_cast<long>(stream_init_result));
        Check(stream_init_result);
        const auto encoder_result = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.GetAddressOf());
        if (FAILED(encoder_result)) trace_sink::EmitHr(L"WIC", L"IWICImagingFactory::CreateEncoder", static_cast<long>(encoder_result));
        Check(encoder_result);
        const auto encoder_init_result = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
        if (FAILED(encoder_init_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapEncoder::Initialize", static_cast<long>(encoder_init_result));
        Check(encoder_init_result);
        const auto frame_result = encoder->CreateNewFrame(frame.GetAddressOf(), options.GetAddressOf());
        if (FAILED(frame_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapEncoder::CreateNewFrame", static_cast<long>(frame_result));
        Check(frame_result);
        const auto frame_init_result = frame->Initialize(options.Get());
        if (FAILED(frame_init_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapFrameEncode::Initialize", static_cast<long>(frame_init_result));
        Check(frame_init_result);
        const auto size_result = frame->SetSize(target_size.width, target_size.height);
        if (FAILED(size_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapFrameEncode::SetSize", static_cast<long>(size_result));
        Check(size_result);
        GUID format = GUID_WICPixelFormat32bppRGBA;
        const auto format_result = frame->SetPixelFormat(&format);
        if (FAILED(format_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapFrameEncode::SetPixelFormat", static_cast<long>(format_result));
        Check(format_result);
        const auto write_result = frame->WriteSource(image.bitmap().Get(), nullptr);
        if (FAILED(write_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapFrameEncode::WriteSource", static_cast<long>(write_result));
        Check(write_result);
        const auto frame_commit_result = frame->Commit();
        if (FAILED(frame_commit_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapFrameEncode::Commit", static_cast<long>(frame_commit_result));
        Check(frame_commit_result);
        const auto encoder_commit_result = encoder->Commit();
        if (FAILED(encoder_commit_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapEncoder::Commit", static_cast<long>(encoder_commit_result));
        Check(encoder_commit_result);
        frame.Reset();
        options.Reset();
        encoder.Reset();
        stream.Reset();
        std::filesystem::rename(temporary, destination);
        return {};
    } catch (std::filesystem::filesystem_error const&) {
        trace_sink::Emit(L"WIC", L"EncodePng filesystem failure destination=" + destination.wstring());
        return Failure(DiagnosticCode::io_failure, destination);
    } catch (HResultError const& error) {
        trace_sink::EmitHr(L"WIC", L"EncodePng failure", static_cast<long>(error.result));
        trace_sink::Emit(L"WIC", L"EncodePng END failure destination=" + destination.wstring());
        return Failure(DiagnosticCode::wic_failure, destination,
                       destination.wstring() + L" (WIC HRESULT " + std::to_wstring(static_cast<uint32_t>(error.result)) + L")");
    } catch (...) {
        trace_sink::Emit(L"WIC", L"EncodePng unknown failure destination=" + destination.wstring());
        return Failure(DiagnosticCode::wic_failure, destination);
    }
}
}
