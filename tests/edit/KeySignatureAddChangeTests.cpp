#include "edit/KeySignatureEdits.h"
#include "support/KeySignatureTestHelpers.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
using Keys = std::vector<KeySignatureChange>;
} // namespace

TEST_CASE("adding a key signature normalizes seven sharps or flats", "[notation][keysig]")
{
    Keys changes;
    KeySignatureEdits::add(changes, 0, 7, false);
    KeySignatureEdits::add(changes, 1920, -7, false);
    KeySignatureEdits::add(changes, 3840, 7, true);
    KeySignatureEdits::add(changes, 5760, -7, true);
    CHECK(changes == Keys{{0, -5, false}, {1920, 5, false}, {3840, -5, true}, {5760, 5, true}});
}

TEST_CASE("adding at an existing tick overwrites it", "[notation][keysig]")
{
    Keys changes;
    KeySignatureEdits::add(changes, 0, 1, false);
    KeySignatureEdits::add(changes, 0, -2, true);
    CHECK(changes == Keys{{0, -2, true}});
}

TEST_CASE("added key signatures are kept in tick order", "[notation][keysig]")
{
    Keys changes;
    KeySignatureEdits::add(changes, 3840, 2, false);
    KeySignatureEdits::add(changes, 0, 0, false);
    KeySignatureEdits::add(changes, 1920, -1, true);
    CHECK(changes == Keys{{0, 0, false}, {1920, -1, true}, {3840, 2, false}});
}
