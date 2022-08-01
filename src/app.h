#pragma once

class IApp {
public:
    virtual void init() {}
    virtual void frame() {}
    virtual void close() {}
    virtual void itemsDropped() {}

    const char *title;
    int width = 800;
    int height = 800;
    int flags = 0;
    int max_dropped_files = 100;
};

enum sapp_keycode;

bool isKeyPressed(sapp_keycode key);
bool isKeyClicked(sapp_keycode key);

extern IApp *app;