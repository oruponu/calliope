#include "model/notation/NoteName.h"

namespace NoteName
{
std::string format(int noteNumber)
{
    static const char* names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return std::string(names[noteNumber % 12]) + std::to_string(noteNumber / 12 - 1);
}
} // namespace NoteName
