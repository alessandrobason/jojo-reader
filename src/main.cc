#include <stdio.h>
#include <math.h>
#include <imgui.h>
#include "tracelog.h"
#include "app.h"
#include "reader.h"

#include <framework.h>

void __sokol_log(const char *msg) {
    debug("SOKOL :> %s", msg);
}

class App : public IApp {
public:
    App() {
        title = "JoJo Reader";
        width = 600;
        height = 600;
        flags = ImGuiConfigFlags_DockingEnable;
    }

    void init() override {
        reader.init();
    }

    void frame() override {
        reader.frame();
    }

    void close() override {
        reader.close();
    }
};

static App myapp;
IApp *app = &myapp;