#pragma once

#include "engine/recording/CountInEnd.h"
#include "engine/recording/PassBoundary.h"
#include "engine/recording/RecordedInput.h"
#include <variant>

using InboxItem = std::variant<RecordedInput, PassBoundary, CountInEnd>;
