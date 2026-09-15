#include "ImagePipeline.h"

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
    Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                           IID_PPV_ARGS(factory.GetAddressOf())));
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
        Check(factory->CreateDecoderFromFilename(source.c_str(), nullptr, GENERIC_READ,
                                                 WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf()));

        ComPtr<IWICBitmapFrameDecode> frame;
        Check(decoder->GetFrame(0, frame.GetAddressOf()));

        ComPtr<IWICBitmapSource> oriented;
        const auto transform = OrientationTransform(frame.Get());
        if (transform == WICBitmapTransformRotate0) {
            Check(frame.As(&oriented));
        } else {
            ComPtr<IWICBitmapFlipRotator> rotator;
            Check(factory->CreateBitmapFlipRotator(rotator.GetAddressOf()));
            Check(rotator->Initialize(frame.Get(), transform));
            Check(rotator.As(&oriented));
        }

        ComPtr<IWICFormatConverter> converter;
        Check(factory->CreateFormatConverter(converter.GetAddressOf()));
        Check(converter->Initialize(oriented.Get(), GUID_WICPixelFormat32bppRGBA,
                                    WICBitmapDitherTypeNone, nullptr, 0.0,
                                    WICBitmapPaletteTypeCustom));

        ComPtr<IWICBitmap> bitmap;
        Check(factory->CreateBitmapFromSource(converter.Get(), WICBitmapCacheOnLoad, bitmap.GetAddressOf()));
        return {DecodedImage::FromBitmap(std::move(bitmap)), {}};
    } catch (HResultError const& error) {
        return {std::nullopt, {DecodeDiagnostic(error.result(), source)}};
    }
}

DecodedImage NormalizeToSquare(DecodedImage const& source) {
    if (source.size_.width == source.size_.height) return source;

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
    return DecodedImage::FromBitmap(std::move(normalized));
}

DecodedImage ResizeRgba(DecodedImage const& source, PixelSize target) {
    EnsureApartment();
    const auto factory = CreateFactory();
    ComPtr<IWICBitmapScaler> scaler;
    Check(factory->CreateBitmapScaler(scaler.GetAddressOf()));
    Check(scaler->Initialize(source.bitmap_.Get(), target.width, target.height, WICBitmapInterpolationModeFant));

    ComPtr<IWICBitmap> resized;
    Check(factory->CreateBitmapFromSource(scaler.Get(), WICBitmapCacheOnLoad, resized.GetAddressOf()));
    return DecodedImage::FromBitmap(std::move(resized));
}
}
