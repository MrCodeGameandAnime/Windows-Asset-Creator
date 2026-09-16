#pragma once

#include <filesystem>

namespace wac {
bool RevealInExplorer(std::filesystem::path const& saved_zip) noexcept;
}
