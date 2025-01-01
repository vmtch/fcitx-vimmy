#include "normal.h"
size_t findNthOccurrence(const std::string &text, char target, int n)
{
    size_t pos = 0;
    int count = 0;

    while ((pos = text.find(target, pos)) != std::string::npos) {
        ++count;
        if (count == n) {
            return pos;
        }
        ++pos;
    }

    return std::string::npos;
}