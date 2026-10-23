#include "support/CopiedControllerPointStringMaker.h"
#include "ui/pianoroll/EditClipboard.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
const CopiedControllerPoints copied{ControllerKind::controlChange(1), {{0, 20, true}, {480, 30, true}}};
}

TEST_CASE("the clipboard holds copied controller points", "[ui][clipboard]")
{
    EditClipboard clipboard;
    clipboard.setControllerPoints(copied);
    CHECK(clipboard.hasControllerPoints());
    CHECK(clipboard.getControllerPoints() == copied);
}

TEST_CASE("copying no controller points empties the clipboard", "[ui][clipboard]")
{
    EditClipboard clipboard;
    clipboard.setNotes({MidiNote{}});
    clipboard.setControllerPoints({ControllerKind::controlChange(1), {}});
    CHECK_FALSE(clipboard.hasNotes());
    CHECK_FALSE(clipboard.hasControllerPoints());
}

TEST_CASE("copying notes replaces controller points", "[ui][clipboard]")
{
    EditClipboard clipboard;
    clipboard.setControllerPoints(copied);
    clipboard.setNotes({MidiNote{}});
    CHECK_FALSE(clipboard.hasControllerPoints());
    CHECK(clipboard.getControllerPoints().points.empty());
}
