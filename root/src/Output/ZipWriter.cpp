#include "ZipWriter.h"

#include <array>
#include <fstream>
#include <windows.h>

namespace wac {
namespace {
void U16(std::ostream& out, uint16_t v) { out.put(static_cast<char>(v)); out.put(static_cast<char>(v >> 8)); }
void U32(std::ostream& out, uint32_t v) { for (auto s : {0,8,16,24}) out.put(static_cast<char>(v >> s)); }
uint32_t Crc(std::vector<uint8_t> const& bytes) { uint32_t crc=0xffffffff; for (auto b:bytes) { crc^=b; for(int i=0;i<8;++i) crc=(crc>>1) ^ (0xedb88320 & -(crc&1)); } return ~crc; }
OperationResult Fail(std::filesystem::path const& p) { return {{{Severity::error, DiagnosticCode::zip_failure, L"Unable to write ZIP archive.", p.wstring()}}}; }
}
OperationResult WriteZip(std::filesystem::path const& root, std::span<std::filesystem::path const> entries, std::filesystem::path const& destination) {
    const auto temporary = std::filesystem::path(destination.wstring() + L".tmp");
    bool temporary_active = false;
    const auto cleanup_temporary = [&]() noexcept {
        if (!temporary_active) return;
        std::error_code error;
        std::filesystem::remove(temporary, error);
        temporary_active = false;
    };
    try {
        if (!destination.parent_path().empty()) std::filesystem::create_directories(destination.parent_path());
        struct Record { std::string name; std::vector<uint8_t> bytes; uint32_t crc; uint32_t offset; };
        std::vector<Record> records;
        for (const auto& entry : entries) {
            const auto file = root / entry;
            if (!std::filesystem::is_regular_file(file)) return Fail(file);
            std::ifstream in(file, std::ios::binary);
            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
            records.push_back({entry.generic_string(), std::move(bytes), 0, 0});
            records.back().crc=Crc(records.back().bytes);
        }
        std::error_code error;
        std::filesystem::remove(temporary, error);
        if (error) return Fail(destination);
        temporary_active = true;
        std::ofstream out(temporary, std::ios::binary|std::ios::trunc); if(!out) { cleanup_temporary(); return Fail(destination); }
        for(auto& r:records) { r.offset=static_cast<uint32_t>(out.tellp()); U32(out,0x04034b50); U16(out,20); U16(out,0); U16(out,0); U16(out,0); U16(out,0); U32(out,r.crc); U32(out,r.bytes.size()); U32(out,r.bytes.size()); U16(out,r.name.size()); U16(out,0); out.write(r.name.data(),r.name.size()); out.write(reinterpret_cast<char const*>(r.bytes.data()),r.bytes.size()); }
        const auto central=static_cast<uint32_t>(out.tellp());
        for(auto const& r:records) { U32(out,0x02014b50); U16(out,20); U16(out,20); U16(out,0); U16(out,0); U16(out,0); U16(out,0); U32(out,r.crc); U32(out,r.bytes.size()); U32(out,r.bytes.size()); U16(out,r.name.size()); U16(out,0); U16(out,0); U16(out,0); U16(out,0); U32(out,0); U32(out,r.offset); out.write(r.name.data(),r.name.size()); }
        const auto end=static_cast<uint32_t>(out.tellp()); U32(out,0x06054b50); U16(out,0); U16(out,0); U16(out,records.size()); U16(out,records.size()); U32(out,end-central); U32(out,central); U16(out,0); out.close();
        if(!out) { cleanup_temporary(); return Fail(destination); }
        if (!MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            cleanup_temporary();
            return Fail(destination);
        }
        temporary_active = false;
        return {};
    } catch (...) {
        cleanup_temporary();
        return Fail(destination);
    }
}
}
