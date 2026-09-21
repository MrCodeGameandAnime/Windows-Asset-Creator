#include "TraceLineBuffer.h"

namespace wac::trace {

void TraceLineBuffer::Enqueue(std::wstring_view line) { lines_.emplace_back(line); }

std::vector<std::wstring> TraceLineBuffer::TakeAll() {
    std::vector<std::wstring> lines;
    lines.swap(lines_);
    return lines;
}

bool TraceLineBuffer::Empty() const noexcept { return lines_.empty(); }

}
