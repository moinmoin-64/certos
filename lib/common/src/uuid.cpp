#include "certosc/common/uuid.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace certosc {

std::string generate_uuid() {
    static thread_local std::mt19937 gen(std::random_device{}());
    static thread_local std::uniform_int_distribution<uint32_t> dist(0, 15);
    static thread_local std::uniform_int_distribution<uint32_t> dist2(8, 11);

    const char* hex = "0123456789abcdef";
    std::string uuid(36, '-');

    // UUID v4 format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            continue; // keep hyphen
        }
        if (i == 14) {
            uuid[i] = '4'; // version 4
        } else if (i == 19) {
            uuid[i] = hex[dist2(gen)]; // variant
        } else {
            uuid[i] = hex[dist(gen)];
        }
    }

    return uuid;
}

} // namespace certosc
