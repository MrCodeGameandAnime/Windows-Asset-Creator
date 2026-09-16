#include "NativeTest.h"
#include "Diagnostics/TraceSink.h"

TEST_CASE(Trace_operation_ids_are_monotonic)
{
    const auto first = wac::trace_sink::NextOperationId();
    const auto second = wac::trace_sink::NextOperationId();
    REQUIRE_EQ(second, first + 1);
}
