#include "app.h"

#include <stdint.h>

#include <imgui.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_imgui.h>
#include <sokol_fetch.h>

#include <windows.h>
#include <stdio.h>

#include <framework.h>

constexpr int keycode_count = SAPP_KEYCODE_MENU + 1;

static bool is_down[keycode_count];
static bool was_down[keycode_count];

static void init(void) {
    initFramework();
    simgui_desc_t simguid{};
    simgui_setup(simguid);
    sfetch_desc_t sd{};
    sd.num_channels = 4;
    sd.num_lanes = 8;
    sfetch_setup(sd);

    memset(is_down, 0, sizeof(is_down));
    memset(was_down, 0, sizeof(was_down));

    ImGui::GetIO().ConfigFlags = app->flags;

    app->init();
}

static void frame(void) {
    sfetch_dowork();

    const simgui_frame_desc_t new_frame = {
        sapp_width(),
        sapp_height(),
        frameTime(),
        sapp_dpi_scale()
    };
    simgui_new_frame(new_frame);
    beginFrame();
    
    app->frame();
    
    simgui_render();
    endFrame();
}

static void cleanup(void) {
    app->close();

    simgui_shutdown();
    sfetch_shutdown();
    cleanupFramework();
}

static void event(const sapp_event* ev) {
    if (simgui_handle_event(ev)) return;

    switch (ev->type) {
    case SAPP_EVENTTYPE_FILES_DROPPED:
        app->itemsDropped();
        break;
    case SAPP_EVENTTYPE_KEY_DOWN:
        is_down[ev->key_code] = true;
        break;
    case SAPP_EVENTTYPE_KEY_UP:
        is_down[ev->key_code] = false;
        break;
    }
}

sapp_desc sokol_main(int argc, char **argv) {
    (void)argc; (void)argv;
    sapp_desc desc{};
    desc.init_cb    = init;
    desc.frame_cb   = frame;
    desc.cleanup_cb = cleanup;
    desc.event_cb   = event;
    desc.enable_dragndrop = true;

    desc.max_dropped_files = app->max_dropped_files;
    desc.window_title = app->title;
    desc.width = app->width;
    desc.height = app->height;

#ifndef NDEBUG
    desc.win32_console_create = true;
#else
    desc.win32_console_attach = true;
#endif
    
    return desc;
}

bool isKeyPressed(sapp_keycode key) {
    return is_down[key];
}

bool isKeyClicked(sapp_keycode key) {
    return is_down[key] && !was_down[key];
}
