// Example CubeVulkan code mod. Built with the engine (target HelloMod), it
// lands in build/examples/HelloMod.dll. Copy it into build/mods to enable it.
//
// Places a ring of street lights around the spawn point; N jumps the clock
// to midnight.
#include <cubevulkan/ModApi.h>
#include <cmath>

static const CwvApi *api;

static int onKey(int key) {
    if (key == 'N') { // GLFW key codes match ASCII for letters.
        api->set_hour(0);
        api->log("Jumped to midnight");
        return 1;
    }
    return 0;
}

extern "C" CWV_EXPORT int cwv_mod_load(const CwvApi *a, CwvMod *mod) {
    if (a->version < 1)
        return 0;
    api = a;
    mod->name = "HelloMod";
    mod->on_key = onKey;
    CwvModel lamp = api->load_model("street-light");
    if (lamp < 0) {
        api->log("street-light.cub not found");
        return 1;
    }
    float focus[3];
    api->get_focus(focus);
    for (int i = 0; i < 8; i++) {
        float angle = i * 0.785398f;
        float x = focus[0] + std::cos(angle) * 9, y = focus[1] + std::sin(angle) * 9;
        api->spawn(lamp, x, y, api->terrain_height(x, y), angle, 1.f / 16);
    }
    api->log("Placed 8 street lights");
    return 1;
}
