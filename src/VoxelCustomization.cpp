#include "VoxelCustomization.h"
#include "GameCraft.h"
#include "NativeInteract.h"
#include "NativePose.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace customization {
namespace {
glm::ivec3 position(upgradewidget::Voxel at) {
    return {at.x, at.y, at.z};
}
bool sameBytes(glm::ivec3 a, glm::ivec3 b) {
    return (uint8_t)a.x == (uint8_t)b.x && (uint8_t)a.y == (uint8_t)b.y && (uint8_t)a.z == (uint8_t)b.z;
}
glm::vec4 material(uint8_t value, float pulse = 1) {
    const float tint[4] = {pulse, pulse, pulse, pulse};
    const auto color = nativepose::materialColour(value, tint, 0);
    return {color[0], color[1], color[2], color[3]};
}
void raw(Plan &plan, Resource resource, const voxeledit::Frame &frame, glm::mat4 model, glm::vec4 color) {
    if (!resource.mesh)
        return;
    guiitem::Request request;
    request.kind = guiitem::Request::Kind::RawEdit;
    request.mesh = resource.mesh;
    request.model = model;
    request.projection = frame.projection;
    request.material = color;
    plan.images.push_back(request);
}
void icon(Plan &plan, Resource resource, const Item &item, guiitem::Placement at, uint32_t cube) {
    if (!resource.mesh)
        return;
    guiitem::Request request;
    request.mesh = resource.mesh;
    request.cubeMesh = cube;
    request.allocation = resource.allocation;
    request.item = item;
    request.at = at;
    plan.images.push_back(request);
}
} // namespace

void State::toggleBench() {
    edit.selected = -1;
    edit.carrying = false;
    edit.open = !edit.open;
}
void State::rotatePointer(glm::vec2 delta, float cameraSpeed) {
    euler.x -= (delta.y * cameraSpeed) * .005f;
    euler.z += (delta.x * cameraSpeed) * .005f;
}
void State::rotatePad(double xDegrees, double zDegrees) {
    euler.x = (float)((double)euler.x + xDegrees);
    euler.z = (float)((double)euler.z + zDegrees);
}
upgradewidget::ClickResult State::press(Item *weapon, bool guiNode, bool &sound) {
    sound = false;
    if (!edit.open || !weapon || !(*weapon)[0])
        return {};
    if (paletteHover >= 0) {
        edit.selected = edit.selected == paletteHover ? -1 : paletteHover;
        sound = true;
        return {};
    }
    if (handHover >= 0) {
        edit.hand = handHover;
        sound = true;
        return {};
    }
    return upgradewidget::click(edit, weapon->data(), guiNode);
}
bool consumePalette(inventoryslots::Inventory &inventory, const Item &item) {
    for (size_t tab = 0; tab < inventory.tabs.size(); ++tab)
        for (size_t slot = 0; slot < inventory.tabs[tab].size(); ++slot) {
            const auto &stack = inventory.tabs[tab][slot];
            if (stack.count != 0 && nativeinteract::sameItem(stack.item.data(), item.data()))
                return inventory.consumeOne({(int)tab, (int)slot});
        }
    return false;
}

Plan draw(State &state, const Layout &layout, const std::array<Item, 2> &equipment, const inventoryslots::Inventory &inventory,
          glm::vec2 pointer, int32_t guiClock, const Resolve &resolve, Resource cube) {
    Plan plan;
    if (!state.edit.open)
        return plan;
    auto &edit = state.edit;
    state.detail = {};
    state.handHover = -1;
    const auto transformed = layout.root * glm::vec4(layout.pivot, 0, 1);
    const auto pivot = glm::vec2(transformed) * (1.0f / transformed.w);
    const glm::vec2 first{(layout.parentSize.x + pivot.x) - 60 * layout.scale, (layout.parentSize.y + pivot.y) - 80 * layout.scale};
    for (int hand = 0; hand < 2; ++hand) {
        const auto &item = equipment[(size_t)hand];
        if (!item[0])
            continue;
        guiitem::Placement at;
        at.screen = first;
        if (hand == 1 && equipment[0][0])
            at.screen.x -= (float)(int32_t)(layout.scale * 100);
        at.euler = {225, 0, guiitem::phase(guiClock)};
        at.screenDepth = -.05f;
        at.size = layout.scale * .075f;
        at.darkPass = true;
        const auto delta = pointer - at.screen;
        if (std::abs(delta.x) < 50 * layout.scale && std::abs(delta.y) < 50 * layout.scale) {
            at.size *= 1.25f;
            state.handHover = hand;
        }
        icon(plan, resolve(item), item, at, cube.mesh);
    }
    if (edit.hand < 0 || edit.hand >= 2 || !equipment[(size_t)edit.hand][0])
        edit.hand = equipment[0][0] ? 0 : equipment[1][0] ? 1 : edit.hand;
    if (edit.hand < 0 || edit.hand >= 2 || !equipment[(size_t)edit.hand][0])
        return plan;
    const auto &weapon = equipment[(size_t)edit.hand];
    edit.palette = upgradewidget::paletteOf(inventory, weapon);
    if (edit.selected >= (int)edit.palette.size())
        edit.selected = (int)edit.palette.size() - 1;
    if (state.paletteHover < 0) {
        if (edit.selectionValid())
            state.detail = edit.palette[(size_t)edit.selected].item;
        else if (edit.carrying)
            state.detail = nativesaves::makeItem(11, edit.carriedMaterial < 128 ? 10 : 14, edit.carriedMaterial);
    }
    const auto resource = resolve(weapon);
    if (resource.volume) {
        const auto &volume = *resource.volume;
        const auto frame =
            voxeledit::frame({volume.sx, volume.sy, volume.sz}, layout.viewport, layout.root, layout.pivot, layout.parentSize, state.euler);
        const auto ray = frame ? voxeledit::pointerRay(*frame, layout.viewport, pointer) : std::nullopt;
        if (frame && ray) {
            auto picked = voxeledit::trace(volume, weapon, *ray, edit.carrying ? std::optional(position(edit.carried)) : std::nullopt);
            if (picked.attributeMaterial) {
                state.detail = nativesaves::makeItem(11, 14, *picked.attributeMaterial);
                state.detail[nativesaves::item::RARITY] = 2;
            }
            raw(plan, resource, *frame, frame->model, glm::vec4(1));
            if (state.paletteHover < 0 && (edit.carrying || edit.selectionValid())) {
                glm::vec3 candidate;
                if (picked.hit) {
                    picked.cell += picked.normal;
                    candidate = glm::vec3(picked.cell);
                } else
                    candidate = ray->origin + ray->direction * 100.0f;
                const auto selectedMaterial = edit.carrying ? edit.carriedMaterial : edit.palette[(size_t)edit.selected].item[0x1c];
                raw(plan, cube, *frame, glm::translate(frame->model, candidate), material(selectedMaterial));
            }
            edit.pointed = picked.hit ? std::optional(upgradewidget::Voxel{picked.cell.x, picked.cell.y, picked.cell.z}) : std::nullopt;
            for (int i = 0; i < std::min<int>(weapon[0x9d], 32); ++i) {
                const auto *entry = weapon.data() + 0x1d + i * 4;
                const glm::ivec3 at{(int8_t)entry[0], (int8_t)entry[1], (int8_t)entry[2]};
                if (edit.carrying && sameBytes(at, position(edit.carried)))
                    continue;
                auto model = glm::translate(frame->model, glm::vec3(at));
                float pulse = 1;
                if (picked.hit && sameBytes(at, picked.cell)) {
                    model = glm::translate(model, glm::vec3(.5f));
                    model = glm::scale(model, glm::vec3(1.1f));
                    model = glm::translate(model, glm::vec3(-.5f));
                    pulse = 1 + .2f * std::cos((float)guiClock * .01f);
                }
                raw(plan, cube, *frame, model, material(entry[3], pulse));
            }
        }
    }
    state.paletteHover = -1;
    for (size_t i = 0; i < edit.palette.size(); ++i) {
        const auto &entry = edit.palette[i];
        const glm::ivec2 corner{(int32_t)(pivot.x + 20 + 45 * (int)(i % 8)), (int32_t)(pivot.y + 40 + 45 * (int)(i / 8))};
        const bool over = pointer.x >= corner.x && pointer.x < corner.x + 40 && pointer.y >= corner.y && pointer.y < corner.y + 40;
        if (over) {
            state.paletteHover = (int)i;
            state.hoveredItem = entry.item;
        }
        guiitem::Placement at;
        at.screen = glm::vec2(corner + 20);
        at.euler = {225, 0, guiitem::phase(guiClock)};
        at.size = over || edit.selected == (int)i ? .15f : .1f;
        at.darkPass = true;
        icon(plan, resolve(entry.item), entry.item, at, cube.mesh);
    }
    plan.upgrades = {{weapon[0x9d], gamecraft::cubeCap(weapon.data())}};
    return plan;
}
} // namespace customization
