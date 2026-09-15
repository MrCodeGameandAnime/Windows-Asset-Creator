#pragma once
#include "../AssetTypes.h"
namespace wac { OperationResult WriteZip(std::filesystem::path const& source_root, std::span<std::filesystem::path const> relative_entries, std::filesystem::path const& destination); }
