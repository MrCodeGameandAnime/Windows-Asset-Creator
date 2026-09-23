#include "ImagePipeline.h"
#include "Diagnostics/TraceSink.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <cwchar>
#include <stdexcept>
#include <string>
#include <vector>

namespace wac {
namespace {
using Microsoft::WRL::ComPtr;

class HResultError final : public std::runtime_error {
public:
    explicit HResultError(HRESULT result) : std::runtime_error("WIC operation failed."), result_(result) {}
    HRESULT result() const noexcept { return result_; }
private:
    HRESULT result_;
};

void Check(HRESULT result) {
    if (FAILED(result)) throw HResultError(result);
}

class Apartment final {
public:
    Apartment() : result_(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) {
        if (FAILED(result_)) trace_sink::EmitHr(L"WIC", L"ImagePipeline CoInitializeEx", static_cast<long>(result_));
        if (FAILED(result_) && result_ != RPC_E_CHANGED_MODE) Check(result_);
    }
    ~Apartment() {
        if (result_ == S_OK || result_ == S_FALSE) CoUninitialize();
    }
private:
    HRESULT result_;
};

void EnsureApartment() {
    static thread_local Apartment apartment;
}

ComPtr<IWICImagingFactory> CreateFactory() {
    ComPtr<IWICImagingFactory> factory;
    const auto result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                         IID_PPV_ARGS(factory.GetAddressOf()));
    if (FAILED(result)) trace_sink::EmitHr(L"WIC", L"CoCreateInstance(CLSID_WICImagingFactory)", static_cast<long>(result));
    Check(result);
    return factory;
}

WICBitmapTransformOptions OrientationTransform(IWICBitmapFrameDecode* frame) {
    ComPtr<IWICMetadataQueryReader> metadata;
    if (FAILED(frame->GetMetadataQueryReader(metadata.GetAddressOf()))) return WICBitmapTransformRotate0;

    PROPVARIANT orientation{};
    const auto result = metadata->GetMetadataByName(L"/app1/ifd/{ushort=274}", &orientation);
    if (FAILED(result) || orientation.vt != VT_UI2) {
        PropVariantClear(&orientation);
        return WICBitmapTransformRotate0;
    }

    const auto value = orientation.uiVal;
    PropVariantClear(&orientation);
    switch (value) {
    case 2: return WICBitmapTransformFlipHorizontal;
    case 3: return WICBitmapTransformRotate180;
    case 4: return WICBitmapTransformFlipVertical;
    case 5: return static_cast<WICBitmapTransformOptions>(WICBitmapTransformRotate270 | WICBitmapTransformFlipHorizontal);
    case 6: return WICBitmapTransformRotate90;
    case 7: return static_cast<WICBitmapTransformOptions>(WICBitmapTransformRotate90 | WICBitmapTransformFlipHorizontal);
    case 8: return WICBitmapTransformRotate270;
    default: return WICBitmapTransformRotate0;
    }
}

bool HasRecognizedImageExtension(std::filesystem::path const& source) {
    const auto extension = source.extension().wstring();
    return _wcsicmp(extension.c_str(), L".png") == 0 ||
           _wcsicmp(extension.c_str(), L".jpg") == 0 ||
           _wcsicmp(extension.c_str(), L".jpeg") == 0;
}

Diagnostic DecodeDiagnostic(HRESULT result, std::filesystem::path const& source) {
    const auto code = result == WINCODEC_ERR_COMPONENTNOTFOUND && !HasRecognizedImageExtension(source)
                          ? DiagnosticCode::unsupported_image
                          : DiagnosticCode::corrupt_image;
    return {Severity::error, code, L"The source image could not be decoded.", source.wstring()};
}
}

DecodedImage::DecodedImage(PixelSize size, ComPtr<IWICBitmap> bitmap)
    : size_(size), bitmap_(std::move(bitmap)) {}

DecodedImage DecodedImage::FromBitmap(ComPtr<IWICBitmap> bitmap) {
    UINT width = 0;
    UINT height = 0;
    Check(bitmap->GetSize(&width, &height));
    return {{width, height}, std::move(bitmap)};
}

PixelSize DecodedImage::size() const noexcept {
    return size_;
}

RgbaPixel DecodedImage::pixel_at(uint32_t x, uint32_t y) const {
    if (x >= size_.width || y >= size_.height) throw std::out_of_range("Pixel position is outside the image.");
    EnsureApartment();
    const WICRect rect{static_cast<INT>(x), static_cast<INT>(y), 1, 1};
    std::array<BYTE, 4> pixel{};
    Check(bitmap_->CopyPixels(&rect, 4, static_cast<UINT>(pixel.size()), pixel.data()));
    return {pixel[0], pixel[1], pixel[2], pixel[3]};
}

ComPtr<IWICBitmap> DecodedImage::bitmap() const noexcept {
    return bitmap_;
}

GenerationResult<DecodedImage> LoadImage(std::filesystem::path const& source) {
    try {
        EnsureApartment();
        const auto factory = CreateFactory();
        ComPtr<IWICBitmapDecoder> decoder;
        const auto decoder_result = factory->CreateDecoderFromFilename(source.c_str(), nullptr, GENERIC_READ,
                                                                         WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf());
        if (FAILED(decoder_result)) trace_sink::EmitHr(L"WIC", L"CreateDecoderFromFilename", static_cast<long>(decoder_result));
        Check(decoder_result);

        ComPtr<IWICBitmapFrameDecode> frame;
        const auto frame_result = decoder->GetFrame(0, frame.GetAddressOf());
        if (FAILED(frame_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapDecoder::GetFrame", static_cast<long>(frame_result));
        Check(frame_result);

        ComPtr<IWICBitmapSource> oriented;
        const auto transform = OrientationTransform(frame.Get());
        if (transform == WICBitmapTransformRotate0) {
            Check(frame.As(&oriented));
        } else {
            ComPtr<IWICBitmapFlipRotator> rotator;
            const auto rotator_result = factory->CreateBitmapFlipRotator(rotator.GetAddressOf());
            if (FAILED(rotator_result)) trace_sink::EmitHr(L"WIC", L"CreateBitmapFlipRotator", static_cast<long>(rotator_result));
            Check(rotator_result);
            const auto transform_result = rotator->Initialize(frame.Get(), transform);
            if (FAILED(transform_result)) trace_sink::EmitHr(L"WIC", L"IWICBitmapFlipRotator::Initialize", static_cast<long>(transform_result));
            Check(transform_result);
            Check(rotator.As(&oriented));
        }

        ComPtr<IWICFormatConverter> converter;
        const auto converter_result = factory->CreateFormatConverter(converter.GetAddressOf());
        if (FAILED(converter_result)) trace_sink::EmitHr(L"WIC", L"CreateFormatConverter", static_cast<long>(converter_result));
        Check(converter_result);
        const auto convert_result = converter->Initialize(oriented.Get(), GUID_WICPixelFormat32bppRGBA,
                                                          WICBitmapDitherTypeNone, nullptr, 0.0,
                                                          WICBitmapPaletteTypeCustom);
        if (FAILED(convert_result)) trace_sink::EmitHr(L"WIC", L"IWICFormatConverter::Initialize", static_cast<long>(convert_result));
        Check(convert_result);

        ComPtr<IWICBitmap> bitmap;
        const auto bitmap_result = factory->CreateBitmapFromSource(converter.Get(), WICBitmapCacheOnLoad, bitmap.GetAddressOf());
        if (FAILED(bitmap_result)) trace_sink::EmitHr(L"WIC", L"CreateBitmapFromSource", static_cast<long>(bitmap_result));
        Check(bitmap_result);
        auto result = DecodedImage::FromBitmap(std::move(bitmap));
        return {std::move(result), {}};
    } catch (HResultError const& error) {
        trace_sink::EmitHr(L"WIC", L"LoadImage failure", static_cast<long>(error.result()));
        trace_sink::Emit(L"WIC", L"LoadImage EXIT failure");
        return {std::nullopt, {DecodeDiagnostic(error.result(), source)}};
    }
}

DecodedImage NormalizeToSquare(DecodedImage const& source) {
    trace_sink::Emit(L"WIC", L"NormalizeToSquare BEGIN dimensions=" + std::to_wstring(source.size_.width) +
                              L"x" + std::to_wstring(source.size_.height));
    if (source.size_.width == source.size_.height) {
        trace_sink::Emit(L"WIC", L"NormalizeToSquare passthrough");
        return source;
    }

    EnsureApartment();
    const auto factory = CreateFactory();
    const auto side = (std::max)(source.size_.width, source.size_.height);
    ComPtr<IWICBitmap> normalized;
    Check(factory->CreateBitmap(side, side, GUID_WICPixelFormat32bppRGBA,
                                WICBitmapCacheOnLoad, normalized.GetAddressOf()));

    ComPtr<IWICBitmapLock> lock;
    Check(normalized->Lock(nullptr, WICBitmapLockWrite, lock.GetAddressOf()));
    UINT buffer_size = 0;
    BYTE* destination = nullptr;
    UINT destination_stride = 0;
    Check(lock->GetDataPointer(&buffer_size, &destination));
    Check(lock->GetStride(&destination_stride));
    std::memset(destination, 0, buffer_size);

    const auto source_stride = source.size_.width * 4;
    std::vector<BYTE> pixels(static_cast<size_t>(source_stride) * source.size_.height);
    Check(source.bitmap_->CopyPixels(nullptr, source_stride, static_cast<UINT>(pixels.size()), pixels.data()));
    const auto x_offset = (side - source.size_.width) / 2;
    const auto y_offset = (side - source.size_.height) / 2;
    for (uint32_t row = 0; row < source.size_.height; ++row) {
        std::memcpy(destination + static_cast<size_t>(row + y_offset) * destination_stride + x_offset * 4,
                    pixels.data() + static_cast<size_t>(row) * source_stride, source_stride);
    }
    auto result = DecodedImage::FromBitmap(std::move(normalized));
    trace_sink::Emit(L"WIC", L"NormalizeToSquare END dimensions=" + std::to_wstring(result.size().width) +
                              L"x" + std::to_wstring(result.size().height));
    return result;
}

DecodedImage ResizeRgba(DecodedImage const& source, PixelSize target) {
    EnsureApartment();
    const auto factory = CreateFactory();

    if (target.width == target.height || target.width == 0 || target.height == 0) {
        ComPtr<IWICBitmapScaler> scaler;
        Check(factory->CreateBitmapScaler(scaler.GetAddressOf()));
        Check(scaler->Initialize(source.bitmap_.Get(), target.width, target.height, WICBitmapInterpolationModeFant));

        ComPtr<IWICBitmap> resized;
        Check(factory->CreateBitmapFromSource(scaler.Get(), WICBitmapCacheOnLoad, resized.GetAddressOf()));
        return DecodedImage::FromBitmap(std::move(resized));
    }

    PixelSize fitted_size{};
    if (static_cast<uint64_t>(target.width) * source.size_.height <=
        static_cast<uint64_t>(target.height) * source.size_.width) {
        fitted_size.width = target.width;
        fitted_size.height = (std::max)(1u, static_cast<uint32_t>(
            static_cast<uint64_t>(source.size_.height) * target.width / source.size_.width));
    } else {
        fitted_size.height = target.height;
        fitted_size.width = (std::max)(1u, static_cast<uint32_t>(
            static_cast<uint64_t>(source.size_.width) * target.height / source.size_.height));
    }

    ComPtr<IWICBitmapScaler> scaler;
    Check(factory->CreateBitmapScaler(scaler.GetAddressOf()));
    Check(scaler->Initialize(source.bitmap_.Get(), fitted_size.width, fitted_size.height,
                             WICBitmapInterpolationModeFant));

    ComPtr<IWICBitmap> fitted;
    Check(factory->CreateBitmapFromSource(scaler.Get(), WICBitmapCacheOnLoad, fitted.GetAddressOf()));

    ComPtr<IWICBitmap> canvas;
    Check(factory->CreateBitmap(target.width, target.height, GUID_WICPixelFormat32bppRGBA,
                                WICBitmapCacheOnLoad, canvas.GetAddressOf()));
    ComPtr<IWICBitmapLock> lock;
    Check(canvas->Lock(nullptr, WICBitmapLockWrite, lock.GetAddressOf()));
    UINT buffer_size = 0;
    BYTE* destination = nullptr;
    UINT destination_stride = 0;
    Check(lock->GetDataPointer(&buffer_size, &destination));
    Check(lock->GetStride(&destination_stride));
    std::memset(destination, 0, buffer_size);

    const auto fitted_stride = fitted_size.width * 4;
    std::vector<BYTE> fitted_pixels(static_cast<size_t>(fitted_stride) * fitted_size.height);
    Check(fitted->CopyPixels(nullptr, fitted_stride, static_cast<UINT>(fitted_pixels.size()),
                             fitted_pixels.data()));
    const auto x_offset = (target.width - fitted_size.width) / 2;
    const auto y_offset = (target.height - fitted_size.height) / 2;
    for (uint32_t row = 0; row < fitted_size.height; ++row) {
        std::memcpy(destination + static_cast<size_t>(row + y_offset) * destination_stride + x_offset * 4,
                    fitted_pixels.data() + static_cast<size_t>(row) * fitted_stride, fitted_stride);
    }

    return DecodedImage::FromBitmap(std::move(canvas));
}
}
