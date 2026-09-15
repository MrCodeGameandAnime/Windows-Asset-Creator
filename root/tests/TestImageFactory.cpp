#include "TestImageFactory.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Microsoft::WRL::ComPtr;

void Check(HRESULT result) {
    if (FAILED(result)) throw std::runtime_error("WIC test-fixture operation failed.");
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

ComPtr<IWICImagingFactory> CreateFactory() {
    ComPtr<IWICImagingFactory> factory;
    Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                           IID_PPV_ARGS(factory.GetAddressOf())));
    return factory;
}

std::filesystem::path FixturePath(std::wstring const& name) {
    const auto directory = std::filesystem::temp_directory_path() / L"WindowsAssetCreator-TestImages";
    std::filesystem::create_directories(directory);
    return directory / name;
}

void WritePng(std::filesystem::path const& path, uint32_t width, uint32_t height,
              std::vector<uint8_t> const& pixels) {
    Apartment apartment;
    const auto factory = CreateFactory();
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> options;
    Check(factory->CreateStream(stream.GetAddressOf()));
    Check(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE));
    Check(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.GetAddressOf()));
    Check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache));
    Check(encoder->CreateNewFrame(frame.GetAddressOf(), options.GetAddressOf()));
    Check(frame->Initialize(options.Get()));
    Check(frame->SetSize(width, height));
    GUID format = GUID_WICPixelFormat32bppRGBA;
    Check(frame->SetPixelFormat(&format));
    ComPtr<IWICBitmap> bitmap;
    Check(factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppRGBA, width * 4,
                                          static_cast<UINT>(pixels.size()),
                                          const_cast<BYTE*>(pixels.data()), bitmap.GetAddressOf()));
    Check(frame->WriteSource(bitmap.Get(), nullptr));
    Check(frame->Commit());
    Check(encoder->Commit());
}

void WriteJpeg(std::filesystem::path const& path, uint32_t width, uint32_t height,
               std::vector<uint8_t> const& rgba, bool rotate_90) {
    Apartment apartment;
    const auto factory = CreateFactory();
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> options;
    Check(factory->CreateStream(stream.GetAddressOf()));
    Check(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE));
    Check(factory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, encoder.GetAddressOf()));
    Check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache));
    Check(encoder->CreateNewFrame(frame.GetAddressOf(), options.GetAddressOf()));

    PROPBAG2 quality_option{};
    quality_option.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
    VARIANT quality{};
    quality.vt = VT_R4;
    quality.fltVal = 1.0F;
    Check(options->Write(1, &quality_option, &quality));

    PROPBAG2 subsampling_option{};
    subsampling_option.pstrName = const_cast<LPOLESTR>(L"JpegYCrCbSubsampling");
    VARIANT subsampling{};
    subsampling.vt = VT_UI1;
    subsampling.bVal = WICJpegYCrCbSubsampling444;
    Check(options->Write(1, &subsampling_option, &subsampling));
    Check(frame->Initialize(options.Get()));
    Check(frame->SetSize(width, height));
    GUID format = GUID_WICPixelFormat24bppBGR;
    Check(frame->SetPixelFormat(&format));

    if (rotate_90) {
        ComPtr<IWICMetadataQueryWriter> metadata;
        Check(frame->GetMetadataQueryWriter(metadata.GetAddressOf()));
        PROPVARIANT orientation{};
        orientation.vt = VT_UI2;
        orientation.uiVal = 6;
        Check(metadata->SetMetadataByName(L"/app1/ifd/{ushort=274}", &orientation));
    }

    ComPtr<IWICBitmap> bitmap;
    Check(factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppRGBA, width * 4,
                                          static_cast<UINT>(rgba.size()),
                                          const_cast<BYTE*>(rgba.data()), bitmap.GetAddressOf()));
    Check(frame->WriteSource(bitmap.Get(), nullptr));
    Check(frame->Commit());
    Check(encoder->Commit());
}

std::vector<uint8_t> Solid(uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
    for (size_t index = 0; index < pixels.size(); index += 4) {
        pixels[index] = r;
        pixels[index + 1] = g;
        pixels[index + 2] = b;
        pixels[index + 3] = a;
    }
    return pixels;
}
}

std::filesystem::path TestImage(std::wstring const& name) {
    const auto path = FixturePath(name);
    if (name == L"wide-red-blue.png") {
        auto pixels = Solid(400, 200, 255, 0, 0, 255);
        for (uint32_t y = 0; y < 200; ++y) {
            for (uint32_t x = 200; x < 400; ++x) {
                const auto index = (static_cast<size_t>(y) * 400 + x) * 4;
                pixels[index] = 0;
                pixels[index + 2] = 255;
            }
        }
        WritePng(path, 400, 200, pixels);
    } else if (name == L"square-alpha.png") {
        WritePng(path, 100, 100, Solid(100, 100, 10, 20, 30, 128));
    } else if (name == L"opaque.jpg") {
        WriteJpeg(path, 40, 20, Solid(40, 20, 20, 150, 230, 255), false);
    } else if (name == L"exif-rotate-90.jpg") {
        auto pixels = Solid(2, 1, 255, 0, 0, 255);
        pixels[4] = 0;
        pixels[5] = 0;
        pixels[6] = 255;
        WriteJpeg(path, 2, 1, pixels, true);
    } else if (name == L"corrupt.png") {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "not a PNG";
    } else {
        throw std::runtime_error("Unknown test image fixture.");
    }
    return path;
}
