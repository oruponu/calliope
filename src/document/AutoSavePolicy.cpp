#include "document/AutoSavePolicy.h"

void AutoSavePolicy::activate(std::uint64_t revision, bool recoveryFileExists)
{
    active = true;
    baseRevision = revision;
    firstChangeSeconds.reset();
    hasRecoveryFile = recoveryFileExists;
}

bool AutoSavePolicy::isActive() const
{
    return active;
}

AutoSaveAction AutoSavePolicy::update(const AutoSaveInputs& inputs)
{
    if (!active)
        return AutoSaveAction::none;

    if (!inputs.hasUnsavedChanges)
    {
        baseRevision = inputs.revision;
        firstChangeSeconds.reset();
        return hasRecoveryFile ? AutoSaveAction::remove : AutoSaveAction::none;
    }

    if (inputs.revision == baseRevision)
        return AutoSaveAction::none;

    if (!firstChangeSeconds)
        firstChangeSeconds = inputs.nowSeconds;

    if (inputs.nowSeconds - *firstChangeSeconds >= intervalSeconds && !inputs.busy)
        return AutoSaveAction::write;
    return AutoSaveAction::none;
}

void AutoSavePolicy::wrote(std::uint64_t revision)
{
    baseRevision = revision;
    firstChangeSeconds.reset();
    hasRecoveryFile = true;
}

void AutoSavePolicy::writeFailed(double nowSeconds)
{
    firstChangeSeconds = nowSeconds;
}

void AutoSavePolicy::removed()
{
    hasRecoveryFile = false;
}
