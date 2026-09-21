#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace wac::trace {

// Buffers early trace output until the development-only external log destination
// has been authorized and is ready. Synchronization belongs to the owner.
class TraceLineBuffer final {
public:
    void Enqueue(std::wstring_view line);
    std::vector<std::wstring> TakeAll();
    bool Empty() const noexcept;

private:
    std::vector<std::wstring> lines_;
};

}
