#pragma once

#include <cstdint>
#include <optional>

enum class AutoSaveAction
{
    none,
    write,
    remove
};

struct AutoSaveInputs
{
    double nowSeconds;
    std::uint64_t revision;
    bool hasUnsavedChanges;
    bool busy;
};

class AutoSavePolicy
{
public:
    static constexpr double intervalSeconds = 300.0;

    void activate(std::uint64_t revision, bool recoveryFileExists);
    bool isActive() const;
    AutoSaveAction update(const AutoSaveInputs& inputs);
    void wrote(std::uint64_t revision);
    void writeFailed(double nowSeconds);
    void removed();

private:
    bool active = false;
    std::uint64_t baseRevision = 0;
    std::optional<double> firstChangeSeconds;
    bool hasRecoveryFile = false;
};
