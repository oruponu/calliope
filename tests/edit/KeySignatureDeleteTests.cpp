#include "edit/KeySignatureEdits.h"
#include "support/KeySignatureStringMaker.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
using Keys = std::vector<KeySignatureChange>;

const Keys before{{0, 0, false}, {1920, 2, false}, {3840, -3, true}};
} // namespace

TEST_CASE("deleting removes the selected key signatures", "[keysig][delete]")
{
    CHECK(KeySignatureEdits::afterDelete(before, {1}) == Keys{{0, 0, false}, {3840, -3, true}});
}

TEST_CASE("several key signatures can be deleted at once", "[keysig][delete]")
{
    CHECK(KeySignatureEdits::afterDelete(before, {0, 2}) == Keys{{1920, 2, false}});
}

TEST_CASE("the first key signature can be deleted", "[keysig][delete]")
{
    CHECK(KeySignatureEdits::afterDelete(before, {0}) == Keys{{1920, 2, false}, {3840, -3, true}});
}

TEST_CASE("out-of-range key signature indices are ignored", "[keysig][delete]")
{
    CHECK(KeySignatureEdits::afterDelete(before, {-1, 3}) == before);
}

TEST_CASE("deleting nothing returns the key signatures unchanged", "[keysig][delete]")
{
    CHECK(KeySignatureEdits::afterDelete(before, {}) == before);
}
