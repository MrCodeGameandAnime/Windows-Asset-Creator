#pragma once

#include "AssetTypes.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace wac {
class DecodedImage final {
public:
    PixelSize size() const noexcept;
    RgbaPixel pixel_at(uint32_t x, uint32_t y) const;
    Microsoft::WRL::ComPtr<IWICBitmap> bitmap() const noexcept;

private:
    friend GenerationResult<DecodedImage> LoadImage(std::filesystem::path const& source);
    friend DecodedImage NormalizeToSquare(DecodedImage const& source);
    friend DecodedImage ResizeRgba(DecodedImage const& source, PixelSize target);

    static DecodedImage FromBitmap(Microsoft::WRL::ComPtr<IWICBitmap> bitmap);
    DecodedImage(PixelSize size, Microsoft::WRL::ComPtr<IWICBitmap> bitmap);

    PixelSize size_{};
    Microsoft::WRL::ComPtr<IWICBitmap> bitmap_;
};

GenerationResult<DecodedImage> LoadImage(std::filesystem::path const& source);
DecodedImage NormalizeToSquare(DecodedImage const& source);
DecodedImage ResizeRgba(DecodedImage const& source, PixelSize target);
}
