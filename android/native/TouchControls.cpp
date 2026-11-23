#include "TouchControls.h"

#include <algorithm>
#include <cmath>

namespace touchcontrols {
namespace {

// XInput's bits; the triggers are whole bytes of their own.
enum : uint16_t {
    DPAD_UP = 0x0001, DPAD_DOWN = 0x0002, DPAD_LEFT = 0x0004, DPAD_RIGHT = 0x0008, START = 0x0010, BACK = 0x0020,
    LEFT_THUMB = 0x0040, LEFT_SHOULDER = 0x0100, RIGHT_SHOULDER = 0x0200, A = 0x1000, B = 0x2000, X = 0x4000, Y = 0x8000
};
enum Trigger { NONE, LEFT_TRIGGER, RIGHT_TRIGGER };

// A button's centre and radius in screen heights, from a corner of the screen.
enum Corner { BOTTOM_RIGHT, TOP_RIGHT, TOP_LEFT };
struct Button {
    const wchar_t *label;
    Corner corner;
    float x, y, radius;
    uint16_t bit;
    Trigger trigger;
};
// What each does follows the controller's mapping (gamepadControls): RB attacks, RT is the special
// attack, B jumps, LB dodges, A uses, LT is the class skill, Y the ultimate, X the quick item;
// Start the system menu, Back the inventory, D-pad left the map, right the lamp, up and down zoom,
// the left stick's click calls the pet.
const Button BUTTONS[] = {
    {L"Attack", BOTTOM_RIGHT, -0.16f, -0.20f, 0.100f, RIGHT_SHOULDER, NONE},
    {L"Special", BOTTOM_RIGHT, -0.36f, -0.13f, 0.075f, 0, RIGHT_TRIGGER},
    {L"Jump", BOTTOM_RIGHT, -0.13f, -0.42f, 0.070f, B, NONE},
    {L"Dodge", BOTTOM_RIGHT, -0.32f, -0.33f, 0.065f, LEFT_SHOULDER, NONE},
    {L"Use", BOTTOM_RIGHT, -0.52f, -0.10f, 0.060f, A, NONE},
    {L"Item", BOTTOM_RIGHT, -0.48f, -0.28f, 0.055f, X, NONE},
    {L"Skill", BOTTOM_RIGHT, -0.30f, -0.53f, 0.060f, 0, LEFT_TRIGGER},
    {L"Ultimate", BOTTOM_RIGHT, -0.12f, -0.62f, 0.060f, Y, NONE},
    {L"Menu", TOP_RIGHT, -0.07f, 0.08f, 0.050f, START, NONE},
    {L"Bag", TOP_RIGHT, -0.19f, 0.08f, 0.050f, BACK, NONE},
    {L"Map", TOP_RIGHT, -0.31f, 0.08f, 0.050f, DPAD_LEFT, NONE},
    {L"Lamp", TOP_RIGHT, -0.43f, 0.08f, 0.050f, DPAD_RIGHT, NONE},
    {L"Pet", TOP_RIGHT, -0.55f, 0.08f, 0.050f, LEFT_THUMB, NONE},
    {L"+", TOP_LEFT, 0.07f, 0.08f, 0.050f, DPAD_UP, NONE},
    {L"-", TOP_LEFT, 0.19f, 0.08f, 0.050f, DPAD_DOWN, NONE},
};
constexpr int COUNT = int(sizeof BUTTONS / sizeof BUTTONS[0]);
constexpr float STICK_RADIUS = 0.12f; // screen heights
constexpr float TOP_ROW = 0.16f;      // the stick does not start in the top row's band

float screenWidth = 1, screenHeight = 1;
bool inWorld = false;
int holder[COUNT];     // the finger holding each button, or -1
int stickFinger = -1;
float stickX = 0, stickY = 0, thumbX = 0, thumbY = 0;
bool initialized = [] {
    std::fill(std::begin(holder), std::end(holder), -1);
    return true;
}();

void centreOf(const Button &b, float &x, float &y) {
    const float h = screenHeight;
    x = (b.corner == TOP_LEFT ? 0 : screenWidth) + b.x * h;
    y = (b.corner == BOTTOM_RIGHT ? screenHeight : 0) + b.y * h;
}

int buttonAt(float x, float y) {
    for (int i = 0; i < COUNT; i++) {
        float cx, cy;
        centreOf(BUTTONS[i], cx, cy);
        const float r = BUTTONS[i].radius * screenHeight * 1.15f; // a little more than drawn, for fingers
        if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r)
            return i;
    }
    return -1;
}
} // namespace

void setScreen(float width, float height) {
    screenWidth = std::max(1.0f, width);
    screenHeight = std::max(1.0f, height);
}

void setWorld(bool world) {
    if (world != inWorld)
        cancel();
    inWorld = world;
}
bool world() { return inWorld; }

bool press(int id, float x, float y) {
    if (!inWorld)
        return false;
    if (const int i = buttonAt(x, y); i >= 0) {
        if (holder[i] < 0)
            holder[i] = id;
        return true;
    }
    if (stickFinger < 0 && x < screenWidth * 0.5f && y > TOP_ROW * screenHeight) {
        stickFinger = id;
        stickX = thumbX = x;
        stickY = thumbY = y;
        return true;
    }
    return false;
}

bool move(int id, float x, float y) {
    if (id == stickFinger) {
        const float r = STICK_RADIUS * screenHeight;
        float dx = x - stickX, dy = y - stickY;
        const float d = std::sqrt(dx * dx + dy * dy);
        if (d > r)
            dx *= r / d, dy *= r / d;
        thumbX = stickX + dx, thumbY = stickY + dy;
        return true;
    }
    return std::find(std::begin(holder), std::end(holder), id) != std::end(holder);
}

bool release(int id) {
    bool taken = false;
    for (int &h : holder)
        if (h == id)
            h = -1, taken = true;
    if (id == stickFinger) {
        stickFinger = -1;
        taken = true;
    }
    return taken;
}

void cancel() {
    std::fill(std::begin(holder), std::end(holder), -1);
    stickFinger = -1;
}

void addTo(androidwindow::Gamepad &pad) {
    if (!inWorld)
        return;
    pad.connected = true;
    for (int i = 0; i < COUNT; i++) {
        if (holder[i] < 0)
            continue;
        pad.buttons |= BUTTONS[i].bit;
        if (BUTTONS[i].trigger == LEFT_TRIGGER)
            pad.leftTrigger = 255;
        if (BUTTONS[i].trigger == RIGHT_TRIGGER)
            pad.rightTrigger = 255;
    }
    if (stickFinger >= 0) {
        const float r = STICK_RADIUS * screenHeight;
        const auto axis = [](float v) { return int16_t(std::clamp(v, -1.0f, 1.0f) * 32767.0f); };
        if (pad.leftX == 0 && pad.leftY == 0) {
            pad.leftX = axis((thumbX - stickX) / r);
            pad.leftY = axis(-(thumbY - stickY) / r); // XInput's Y points up
        }
    }
}

std::vector<androidwindow::TouchShape> shapes() {
    std::vector<androidwindow::TouchShape> out;
    if (!inWorld)
        return out;
    // The menus' look: a dark translucent face with a light rim, brighter while held.
    for (int i = 0; i < COUNT; i++) {
        float x, y;
        centreOf(BUTTONS[i], x, y);
        const bool held = holder[i] >= 0;
        out.push_back({x, y, BUTTONS[i].radius * screenHeight, held ? 0.47f : 0.27f, held ? 0.92f : 0.66f, BUTTONS[i].label});
    }
    if (stickFinger >= 0) {
        const float r = STICK_RADIUS * screenHeight;
        out.push_back({stickX, stickY, r, 0.20f, 0.45f, nullptr});
        out.push_back({thumbX, thumbY, r * 0.38f, 0.65f, 0.85f, nullptr});
    }
    return out;
}

} // namespace touchcontrols
