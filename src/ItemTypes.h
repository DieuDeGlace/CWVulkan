#pragma once
// Inventory type rules shared by generated items and complete owner records.
// Independent contract: steam-inventory-slot-lifecycle §§1–2.
#include <cstdint>

namespace itemtypes {
constexpr int inventoryTab(uint8_t type) {
    switch (type) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 15:
    case 25:
    case 26:
        return 0;
    case 22:
    case 24:
    case 29:
        return 1;
    case 2:
    case 11:
        return 3;
    case 20:
    case 21:
        return 4;
    case 23:
        return 5;
    default:
        return 2;
    }
}
constexpr bool stackable(uint8_t type) {
    switch (type) {
    case 1:
    case 10:
    case 11:
    case 12:
    case 13:
    case 22:
    case 28:
        return true;
    default:
        return false;
    }
}
} // namespace itemtypes
