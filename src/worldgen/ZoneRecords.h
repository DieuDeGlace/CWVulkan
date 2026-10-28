#pragma once
// The records a zone holds besides its blocks (docs/specs/worldgen-structures.md §4): static
// objects (zone +0x60), models (+0x18) and items lying about (+0x30), with the item record they
// carry (drops-and-items.md §0.1).
#include "worldgen/Regions.h"

#include <cstdint>

namespace worldgen {

// Record offsets, for the readers of extracted zones.
namespace zonelist {
constexpr uint32_t MODELS = 0x18;
constexpr uint32_t ITEMS = 0x30;
constexpr uint32_t ITEMS_COPY = 0x48;
constexpr uint32_t OBJECTS = 0x60;
constexpr uint32_t OBJECTS_COPY = 0x78;
} // namespace zonelist

// An item (the game's 0xa0-byte item record, the fields the generator sets).
struct Item {
    uint8_t category = 0; // +0x00; 0 for none
    int32_t id = 0;       // +0x04
    uint32_t modifier = 0; // +0x08: a generated item's seed
    Int2 region;          // +0x0c, +0x10
    uint8_t rarity = 0;   // +0x14: the rarity, or a worn item's level
    uint32_t realm = 0;   // +0x18: an artifact's realm
    uint8_t material = 0; // +0x1c
};

// A static object (0xe0 bytes; made by 0x2f93a0).
struct StaticObject {
    int32_t type = 0;          // +0x00
    int64_t x = 0, y = 0, z = 0; // +0x08..: 1/65536 blocks, z at the object's foot
    uint8_t turn = 0;          // +0x20: quarter turns
    float scale = 1.0f;        // +0x24
    bool active = true;        // +0x28
    float swing = 0.0f;        // +0x2c: 1000.0 for doors, window panes and beams
    int64_t owner = 0;         // +0x30
    Item takes;                // +0x38: the item it takes (empty)
    uint8_t takesCount = 1;    // +0xd8
};

// A model (0x40 bytes): plants, furniture, lights, signs.
struct Model {
    int32_t type = 0;          // +0x00
    int64_t x = 0, y = 0, z = 0; // +0x08..
    float size = 0.0f;         // +0x20
    float yaw = 0.0f;          // +0x24, degrees
    float r = 0, g = 0, b = 0; // +0x2c..: tint
    int32_t flags = 0;         // +0x38: 1 gives light
};
namespace modelflag {
constexpr int32_t LIGHT = 1;
}

// An item lying about (0xe0 bytes; made by 0x118c20).
struct GroundItem {
    Item item;
    int64_t x = 0, y = 0, z = 0; // +0xa0..
    float angle = 0.0f;          // +0xc0, degrees
    uint32_t flags = 0;          // +0xc4: bit 1 a treasure each player finds for themselves
};
constexpr uint32_t PER_PLAYER = 2;

// Item categories the generator lays down.
namespace itemcategory {
constexpr uint8_t PLANT = 0xb;   // herbs, shells, crops, water flasks
constexpr uint8_t BOOK = 0x13;
constexpr uint8_t SPECIAL = 24;  // role items, boats
constexpr uint8_t GENERATED = 30; // a reward: weapon, armour or the like made from its modifier
} // namespace itemcategory

// Static object types the generator refers to by number.
namespace objecttype {
constexpr int32_t CHEST = 0x16;
constexpr int32_t CELL_92_OBJECT = 0x75; // beside a type-92 building; spots of kind 3 stand on it
constexpr int32_t MENHIR = 0x7d;
constexpr int32_t CAMPFIRE = 0x53;
} // namespace objecttype

} // namespace worldgen
