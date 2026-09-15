#include "IcoWriter.h"
#include "PngEncoder.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace wac {
namespace {
void Put16(std::ostream& output, uint16_t value) { output.put(static_cast<char>(value)); output.put(static_cast<char>(value >> 8)); }
void Put32(std::ostream& output, uint32_t value) { for (auto shift : {0, 8, 16, 24}) output.put(static_cast<char>(value >> shift)); }
std::vector<uint8_t> ReadBytes(std::filesystem::path const& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
OperationResult Failure(std::filesystem::path const& path) {
    return {{{Severity::error, DiagnosticCode::io_failure, L"Unable to write ICO asset.", path.wstring()}}};
}
}

OperationResult WriteAppIcon(DecodedImage const& normalized_source, std::filesystem::path const& destination) {
    try {
        constexpr std::array<uint32_t, 5> sizes{16, 24, 32, 48, 256};
        std::filesystem::create_directories(destination.parent_path());
        std::vector<std::vector<uint8_t>> payloads;
        for (const auto size : sizes) {
            const auto png = destination.wstring() + L"." + std::to_wstring(size) + L".png";
            const auto result = EncodePng(normalized_source, png, {size, size});
            if (!result.succeeded()) return result;
            auto payload = ReadBytes(png);
            std::filesystem::remove(png);
            if (payload.size() < 8 || payload[0] != 0x89 || payload[1] != 0x50 || payload[2] != 0x4e || payload[3] != 0x47) return Failure(destination);
            payloads.push_back(std::move(payload));
        }
        const auto temporary = destination.wstring() + L".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return Failure(destination);
        Put16(output, 0); Put16(output, 1); Put16(output, static_cast<uint16_t>(sizes.size()));
        uint32_t offset = 6 + static_cast<uint32_t>(sizes.size()) * 16;
        for (size_t index = 0; index < sizes.size(); ++index) {
            output.put(static_cast<char>(sizes[index] == 256 ? 0 : sizes[index]));
            output.put(static_cast<char>(sizes[index] == 256 ? 0 : sizes[index]));
            output.put(0); output.put(0); Put16(output, 1); Put16(output, 32);
            Put32(output, static_cast<uint32_t>(payloads[index].size())); Put32(output, offset);
            offset += static_cast<uint32_t>(payloads[index].size());
        }
        for (const auto& payload : payloads) output.write(reinterpret_cast<char const*>(payload.data()), payload.size());
        output.close();
        if (!output) return Failure(destination);
        std::filesystem::rename(temporary, destination);
        return {};
    } catch (...) { return Failure(destination); }
}
}
