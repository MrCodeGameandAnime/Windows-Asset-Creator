#pragma once

#include "../StoreMsixProfile.h"

namespace wac {
class GenerationSession final {
public:
    GenerationSession(std::filesystem::path staging_root, StoreMsixProfile profile, std::vector<GeneratedAsset> assets);
    GenerationSession(GenerationSession&& other) noexcept;
    GenerationSession& operator=(GenerationSession&& other) noexcept;
    ~GenerationSession();
    std::filesystem::path const& staging_root() const noexcept;
    StoreMsixProfile const& profile() const noexcept;
    std::span<GeneratedAsset const> preview_assets() const noexcept;
    OperationResult ExportZip(std::filesystem::path const& user_destination) const;
private:
    std::filesystem::path staging_root_;
    StoreMsixProfile profile_;
    std::vector<GeneratedAsset> assets_;
};
}
