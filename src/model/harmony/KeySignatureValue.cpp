#include "model/harmony/KeySignatureValue.h"

int KeySignatureValue::normalizeSharpsOrFlats(int sharpsOrFlats)
{
    if (sharpsOrFlats == 7)
        return -5;
    if (sharpsOrFlats == -7)
        return 5;
    return sharpsOrFlats;
}
