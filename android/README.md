# CubeVulkan on Android

The Android app runs the same engine and the same `src/main.cpp` as `CubeVulkan.exe`, built for
ARM64 with the Vulkan renderer. Android supplies the window, touch, keyboard, game controller
and storage through a small GLFW-compatible layer (`native/AndroidGlfw.cpp`), so the engine's
input, key bindings and options read the same as on Windows. It is single player: no Steam
client runs on Android, so it starts as `CubeVulkan.exe --offline` does.

**Status: early port.** It builds and packages, but it has not yet run on a device. There are
no on-screen movement or action buttons yet: play with a game controller or a keyboard.

## Requirements

- A Windows desktop build of CubeVulkan first (see the main README): the Android build compiles
  its shaders with the desktop `build\spirv_compile.exe` (or set `CUBEVULKAN_HOST_SPIRV_COMPILE`).
- Java 17 and Gradle 8.11.1.
- Android SDK platform 35, build-tools 35, CMake 3.31.6 and NDK 30.0.16248370.
- Git on the path (the first configure fetches the same pinned sources as the desktop build, plus
  GLFW's header and Mesa's GLU tessellator).
- A phone or tablet with Android 13 or newer (ARM64) and a Vulkan 1.3 driver with dynamic
  rendering.

## Build

```
set ANDROID_HOME=<Android SDK folder>
set CUBEVULKAN_ANDROID_BUILD=C:\cvg
gradle assembleDebug
```

Run it in this folder. The APK is written to `build/outputs/apk/debug/`.
`CUBEVULKAN_ANDROID_BUILD` places the native build in a short folder, because the fetched
dependencies can pass Windows' 260-character path limit below a long checkout path.

`build-native.cmd` builds only the native library (with `ANDROID_NDK_HOME` set), which is quicker
for checking that the engine still compiles for Android.

## Game data

The app uses your own Steam Cube World, prepared on Windows as for the desktop engine (the
`Extract` folder from CubeVulkanSetup's extraction must be present). Copy the game folder to the
phone, for example into Documents. On first start the app asks for it: tap **Choose folder** and
pick the folder that contains `cubeworld.exe`, `sprites` and `Extract`. It is copied once into
the app's own folder as `Android/data/org.cubevulkan.app/files/Cube World` (about 500 MB). The
chosen folder is only read.

The app keeps `CubeVulkan.ini`, `CubeVulkan.log`, `options.cfg`, `controls.cfg` and `Save/` in
`Android/data/org.cubevulkan.app/files/`. Over USB, the game folder can also be pushed there:

```
adb push "Cube World" /sdcard/Android/data/org.cubevulkan.app/files/
```

After a crash, start the app once more to write `log_crash.txt` in the same folder.

## Playing

- In menus a finger is the mouse: it points, then clicks. A second finger holds the right button
  (it drags the map).
- While the game holds the cursor (the camera), dragging turns the camera.
- A hardware keyboard works as on Windows, with the same bindings.
- A game controller works as an Xbox controller does on Windows.
- Back is Escape.

## Differences from Windows

- No Steam: no multiplayer through Steam, no Steam Cloud, and no ownership check (the game folder
  is your own copy).
- One display mode: the device's screen.
- Native mods are not loaded from the app's folder (Android does not run libraries from shared
  storage). Lua mods and asset overrides in `mods/` work.
- Plain `char` is signed and floating-point operations are not contracted, as with MSVC, so a seed
  generates the same world; the standard library can still differ in details. The interface's
  outline shapes are filled by Mesa's GLU tessellator in place of Windows' glu32 (the same SGI
  algorithm).
