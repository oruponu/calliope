#include "model/harmony/KeySignatureValue.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("seven sharps or flats are normalized to the simpler spelling", "[notation][keysig]")
{
    CHECK(KeySignatureValue::normalizeSharpsOrFlats(7) == -5);
    CHECK(KeySignatureValue::normalizeSharpsOrFlats(-7) == 5);
}

TEST_CASE("other sharps or flats are unchanged", "[notation][keysig]")
{
    for (int sf = -6; sf <= 6; ++sf)
    {
        CAPTURE(sf);
        CHECK(KeySignatureValue::normalizeSharpsOrFlats(sf) == sf);
    }
}
