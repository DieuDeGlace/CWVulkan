#pragma once
// The on-screen controls of the Android app, a CubeVulkan addition with no Steam counterpart: a
// floating stick on the left half of the screen and round buttons on the right, shown while the
// game holds the cursor (in the world). They act as a game controller in XInput's layout, so the
// engine reads them as it reads an Xbox controller on Windows (GameInput.cpp's gamepadControls).
#include "AndroidWindow.h"

#include <string>
#include <vector>

namespace touchcontrols {

void setScreen(float width, float height);
// The world (the game holds the cursor): the controls show and take touches. Leaving it lets go.
void setWorld(bool world);
bool world();

// A finger arriving: true when a control takes it (a button, or the stick on the left half).
bool press(int id, float x, float y);
// True when the finger is a control's (its motion is not the camera's).
bool move(int id, float x, float y);
bool release(int id);
void cancel();

// The controls' held buttons and stick, added to a controller's state.
void addTo(androidwindow::Gamepad &pad);

std::vector<androidwindow::TouchShape> shapes();

} // namespace touchcontrols
