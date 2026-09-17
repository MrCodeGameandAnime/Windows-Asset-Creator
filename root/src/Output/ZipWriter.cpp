#include "ZipWriter.h"

#include "../Diagnostics/TraceSink.h"

#include <array>
#include <cerrno>
#include <fstream>
#include <windows.h>

namespace wac {
namespace {
void U16(std::ostream& out, uint16_t v) { out.put(static_cast<char>(v)); out.put(static_cast<char>(v >> 8)); }
void U32(std::ostream& out, uint32_t v) { for (auto s : {0,8,16,24}) out.put(static_cast<char>(v >> s)); }
uint32_t Crc(std::vector<uint8_t> const& bytes) { uint32_t crc=0xffffffff; for (auto b:bytes) { crc^=b; for(int i=0;i<8;++i) crc=(crc>>1) ^ (0xedb88320 & -(crc&1)); } return ~crc; }
std::wstring Widen(std::string const& value) { return {value.begin(), value.end()}; }
std::wstring ErrorCodeText(std::error_code const& error) {
    return L"value=" + std::to_wstring(error.value()) + L" message=" + Widen(error.message());
}
std::wstring FilesystemErrorText(std::filesystem::filesystem_error const& error) {
    return L"code=" + ErrorCodeText(error.code()) + L" path1=" + error.path1().wstring() + L" path2=" +
           error.path2().wstring() + L" what=" + Widen(error.what());
}
OperationResult Fail(std::filesystem::path const& p) { return {{{Severity::error, DiagnosticCode::zip_failure, L"Unable to write ZIP archive.", p.wstring()}}}; }
}
OperationResult WriteZip(std::filesystem::path const& root, std::span<std::filesystem::path const> entries, std::filesystem::path const& destination) {
    const auto temporary = std::filesystem::path(destination.wstring() + L".tmp");
    trace_sink::Emit(L"ZIP", L"WriteZip ENTER destination=" + destination.wstring() + L" temporary=" + temporary.wstring() +
                              L" root=" + root.wstring() + L" entries=" + std::to_wstring(entries.size()));
    bool temporary_active = false;
    const auto cleanup_temporary = [&]() noexcept {
        if (!temporary_active) return;
        std::error_code error;
        std::filesystem::remove(temporary, error);
        temporary_active = false;
    };
    try {
        if (!destination.parent_path().empty()) {
            try {
                std::filesystem::create_directories(destination.parent_path());
                trace_sink::Emit(L"ZIP", L"destination.parent_path create_directories success error_code=value=0 message=");
            } catch (const std::filesystem::filesystem_error& error) {
                trace_sink::Emit(L"ZIP", L"destination.parent_path create_directories failure " + FilesystemErrorText(error));
                throw;
            }
        }
        struct Record { std::string name; std::vector<uint8_t> bytes; uint32_t crc; uint32_t offset; };
        std::vector<Record> records;
        std::uint64_t total_bytes = 0;
        bool all_reads_successful = true;
        for (const auto& entry : entries) {
            const auto file = root / entry;
            if (!std::filesystem::is_regular_file(file)) {
                trace_sink::Emit(L"ZIP", L"source collection failure missing_or_nonregular=" + file.wstring());
                return Fail(file);
            }
            std::ifstream in(file, std::ios::binary);
            const bool opened = in.is_open();
            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
            if (!opened || in.bad()) all_reads_successful = false;
            total_bytes += bytes.size();
            records.push_back({entry.generic_string(), std::move(bytes), 0, 0});
            records.back().crc=Crc(records.back().bytes);
        }
        trace_sink::Emit(L"ZIP", L"source collection complete files=" + std::to_wstring(records.size()) +
                                  L" bytes=" + std::to_wstring(total_bytes) + L" all_reads_successful=" +
                                  std::wstring{all_reads_successful ? L"true" : L"false"});
        std::error_code error;
        const bool removed = std::filesystem::remove(temporary, error);
        trace_sink::Emit(L"ZIP", L"temporary remove before write removed=" + std::wstring{removed ? L"true" : L"false"} +
                                  L" " + ErrorCodeText(error));
        if (error) return Fail(destination);
        temporary_active = true;
        trace_sink::Emit(L"ZIP", L"temporary ofstream open BEGIN path=" + temporary.wstring());
        std::ofstream out(temporary, std::ios::binary|std::ios::trunc);
        if (!out) {
            const int open_errno = errno;
            const auto open_last_error = GetLastError();
            trace_sink::Emit(L"ZIP", L"temporary ofstream open FAILURE errno=" + std::to_wstring(open_errno) +
                                      L" GetLastError=" + std::to_wstring(open_last_error) +
                                      L" path=" + temporary.wstring());
            cleanup_temporary();
            return Fail(destination);
        }
        trace_sink::Emit(L"ZIP", L"temporary ofstream open SUCCESS path=" + temporary.wstring());
        for(auto& r:records) { r.offset=static_cast<uint32_t>(out.tellp()); U32(out,0x04034b50); U16(out,20); U16(out,0); U16(out,0); U16(out,0); U16(out,0); U32(out,r.crc); U32(out,r.bytes.size()); U32(out,r.bytes.size()); U16(out,r.name.size()); U16(out,0); out.write(r.name.data(),r.name.size()); out.write(reinterpret_cast<char const*>(r.bytes.data()),r.bytes.size()); }
        const auto central=static_cast<uint32_t>(out.tellp());
        for(auto const& r:records) { U32(out,0x02014b50); U16(out,20); U16(out,20); U16(out,0); U16(out,0); U16(out,0); U16(out,0); U32(out,r.crc); U32(out,r.bytes.size()); U32(out,r.bytes.size()); U16(out,r.name.size()); U16(out,0); U16(out,0); U16(out,0); U16(out,0); U32(out,0); U32(out,r.offset); out.write(r.name.data(),r.name.size()); }
        const auto end=static_cast<uint32_t>(out.tellp()); U32(out,0x06054b50); U16(out,0); U16(out,0); U16(out,records.size()); U16(out,records.size()); U32(out,end-central); U32(out,central); U16(out,0); out.close();
        const bool stream_good_after_close = static_cast<bool>(out);
        trace_sink::Emit(L"ZIP", L"ZIP write/close stream_good=" +
                                  std::wstring{stream_good_after_close ? L"true" : L"false"});
        if (!stream_good_after_close) { cleanup_temporary(); return Fail(destination); }
        trace_sink::Emit(L"ZIP", L"MoveFileExW BEGIN temporary=" + temporary.wstring() + L" destination=" + destination.wstring());
        if (!MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            const auto move_last_error = GetLastError();
            trace_sink::Emit(L"ZIP", L"MoveFileExW FAILURE GetLastError=" + std::to_wstring(move_last_error));
            cleanup_temporary();
            return Fail(destination);
        }
        trace_sink::Emit(L"ZIP", L"MoveFileExW SUCCESS");
        temporary_active = false;
        return {};
    } catch (const std::filesystem::filesystem_error& error) {
        trace_sink::Emit(L"ZIP", L"catch filesystem_error " + FilesystemErrorText(error));
        cleanup_temporary();
        return Fail(destination);
    } catch (...) {
        trace_sink::Emit(L"ZIP", L"catch unknown exception");
        cleanup_temporary();
        return Fail(destination);
    }
}
}
