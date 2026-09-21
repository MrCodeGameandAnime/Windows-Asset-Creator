#include "NativeTest.h"
#include "Diagnostics/TraceLineBuffer.h"
#include "Diagnostics/TraceSink.h"

#include <string>
#include <vector>

TEST_CASE(Trace_operation_ids_are_monotonic)
{
    const auto first = wac::trace_sink::NextOperationId();
    const auto second = wac::trace_sink::NextOperationId();
    REQUIRE_EQ(second, first + 1);
}

TEST_CASE(Trace_line_buffer_preserves_early_log_lines_until_destination_is_ready)
{
    wac::trace::TraceLineBuffer pending;
    pending.Enqueue(L"session started");
    pending.Enqueue(L"window activated");

    auto early_lines = pending.TakeAll();
    REQUIRE_EQ(early_lines.size(), size_t{2});
    REQUIRE_EQ(early_lines[0], std::wstring{L"session started"});
    REQUIRE_EQ(early_lines[1], std::wstring{L"window activated"});

    pending.Enqueue(L"generation ready");
    const auto later_lines = pending.TakeAll();
    REQUIRE_EQ(later_lines.size(), size_t{1});
    REQUIRE_EQ(later_lines[0], std::wstring{L"generation ready"});
    REQUIRE_EQ(pending.TakeAll().size(), size_t{0});
}
