#include "PhysicalInput.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <cstdio>
#include <limits>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <dinput.h>
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace physicalinput {
namespace {
int32_t truncate(float value) {
    if (!(value >= -2147483648.f && value < 2147483648.f))
        return std::numeric_limits<int32_t>::min();
    return int32_t(value);
}
void store(const Current &current, int index, uint8_t value) {
    (*current().levels)[size_t(index)] = value;
}
} // namespace

void Editor::bind(gameoptions::Binding binding) {
    if (uint32_t(waiting) >= values.size())
        return;
    for (auto &old : values)
        if (old.kind == binding.kind && old.code == binding.code)
            old = {0, 0};
    values[size_t(waiting)] = binding;
    candidate = -1;
}

void Editor::release(int32_t button) {
    waiting = -1;
    if (visible && button == 0 && candidate >= 0)
        waiting = candidate;
}

void Editor::beginDraw(bool padSelected) {
    candidate = -1;
    if (padSelected)
        waiting = -1;
}

bool Editor::offerRow(int row, float baseline, glm::vec2 point, float width, bool padSelected) {
    if (waiting >= 0 || padSelected || !(point.x >= 0) || !(point.x < width - 40.f) ||
        !(point.y >= baseline - 20.f) || !(point.y < baseline + 10.f))
        return false;
    candidate = row;
    return true;
}

int Editor::visibleRows(float height) {
    const float body = height - 120.f;
    return truncate(body) / 30;
}

std::array<int32_t, 2> Editor::rows(float height) const {
    const int32_t end = std::bit_cast<int32_t>(uint32_t(visibleRows(height)) + uint32_t(first));
    return {std::clamp(first, 0, 23), std::clamp(end, 0, 23)};
}

void Poll::sample(bool admitted, const ReadKeyboard &readKeyboard, const ReadMouse &readMouse, const Current &current) {
    phase = Skipped;
    if (!admitted)
        return;
    readKeyboard(keyboard);
    readMouse(mouse);
    const bool capturing = current().editor->waiting >= 0;
    if (capturing) {
        phase = Capturing;
        for (int key = 0; key < int(keyboard.size()); ++key) {
            if (oldKeyboard[size_t(key)] == 0 && keyboard[size_t(key)] != 0) {
                if (key != 1 && key != 0x1C && key != 0x9C)
                    current().editor->bind({key, 0});
                current().editor->waiting = -1;
                break;
            }
            // This repeated scan is observable through each fresh receiver load.
            // It must remain inside the keyboard loop, even for identical edges.
            for (int button = 0; button < 4; ++button)
                if (oldMouse[size_t(12 + button)] == 0 && mouse[size_t(12 + button)] != 0)
                    current().editor->bind({button, 1});
        }
        return;
    }
    phase = Normal;
    for (int index = 0; index < gameoptions::BINDINGS; ++index) {
        store(current, index, 0);
        const auto &binding = (*current().bindings)[size_t(index)];
        uint8_t value = 0;
        if (binding.kind == 0) {
            if (binding.code == 0)
                continue;
            const uint32_t code = uint32_t(binding.code);
            if (code < keyboard.size())
                value = keyboard[code] != 0;
        } else if (binding.kind == 1) {
            const uint32_t code = uint32_t(binding.code);
            if (code < 4)
                value = mouse[12 + code] != 0;
        } else
            continue;
        store(current, index, value);
        if (value)
            *current().padSelected = false;
    }
}

void Poll::clearZoom(const Current &current) {
    if (phase == Normal) {
        store(current, gameoptions::ZOOM_IN, 0);
        store(current, gameoptions::ZOOM_OUT, 0);
    }
}

void Poll::finish() {
    if (phase != Skipped) {
        oldKeyboard = keyboard;
        oldMouse = mouse;
    }
}

glm::vec2 Poll::relativeMotion() const {
    int32_t xy[2];
    std::memcpy(xy, mouse.data(), sizeof xy);
    return {float(xy[0]), float(xy[1])};
}

struct Devices::State {
    HWND window = nullptr;
    GLFWwindow *glfwWindow = nullptr;
    HRESULT mouseCreation = 0;
    double previousX = 0, previousY = 0;
    bool previousPoint = false;
    IDirectInput8W *input = nullptr;
    IDirectInputDevice8W *keyboard = nullptr, *mouse = nullptr;
    ~State() {
        if (keyboard) keyboard->Release();
        if (mouse) mouse->Release();
        if (input) input->Release();
    }
};

Devices::Devices() = default;
Devices::~Devices() = default;

bool Devices::open(GLFWwindow *window, std::string &error) {
    close();
    auto next = std::make_unique<State>();
    next->window = glfwGetWin32Window(window);
    next->glfwWindow = window;
    const HRESULT created = DirectInput8Create(GetModuleHandleW(nullptr), 0x0800, IID_IDirectInput8W,
                                               reinterpret_cast<void **>(&next->input), nullptr);
    if (created != 0 || !next->input) {
        error = "Could not initialize Windows physical input";
        return false;
    }
    next->input->CreateDevice(GUID_SysKeyboard, &next->keyboard, nullptr);
    if (!next->keyboard) { // Defined engine diagnostic outside the native valid graph.
        error = "Windows did not supply a physical keyboard device";
        return false;
    }
    next->keyboard->SetDataFormat(&c_dfDIKeyboard);
    next->keyboard->Acquire();
    next->mouseCreation = next->input->CreateDevice(GUID_SysMouse, &next->mouse, nullptr);
    if (next->mouse) {
        next->mouse->SetDataFormat(&c_dfDIMouse);
        next->mouse->SetCooperativeLevel(next->window, 10);
        next->mouse->Acquire();
    }
    // A missing Windows mouse provider is outside the valid native graph. Keep
    // the explicit existing GLFW compatibility backend available on such hosts.
    // A real DirectInput device's failed reads are still consumed without clears,
    // retries, reacquisition or replacement by this fallback.
    state = std::move(next);
    return true;
}

void Devices::close() { state.reset(); }
bool Devices::foreground() const { return state && GetForegroundWindow() == state->window; }
std::string Devices::description() const {
    if (!state) return "not initialized";
    if (state->mouse) return "Unicode DirectInput keyboard and mouse";
    char message[128];
    std::snprintf(message, sizeof message, "Unicode DirectInput keyboard; GLFW compatibility mouse (DirectInput creation 0x%08lX)",
                  static_cast<unsigned long>(state->mouseCreation));
    return message;
}
void Devices::readKeyboard(Keyboard &output) {
    if (state) state->keyboard->GetDeviceState(DWORD(output.size()), output.data());
}
void Devices::readMouse(Mouse &output) {
    static_assert(sizeof(DIMOUSESTATE) == 16);
    if (!state) return;
    if (state->mouse) {
        state->mouse->GetDeviceState(DWORD(output.size()), output.data());
        return;
    }
    double x, y;
    glfwGetCursorPos(state->glfwWindow, &x, &y);
    const int32_t motion[3] = {
        state->previousPoint ? truncate(float(x - state->previousX)) : 0,
        state->previousPoint ? truncate(float(y - state->previousY)) : 0,
        0
    };
    std::memcpy(output.data(), motion, sizeof motion);
    for (int button = 0; button < 4; ++button)
        output[size_t(12 + button)] = glfwGetMouseButton(state->glfwWindow, button) == GLFW_PRESS ? 0x80 : 0;
    state->previousX = x;
    state->previousY = y;
    state->previousPoint = true;
}
void Devices::inactiveWait() { Sleep(16); }
} // namespace physicalinput
