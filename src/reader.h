#pragma once

#include <thread>
#include <atomic>

#include <imgui.h>

#include "utils/vec.h"
#include "utils/str.h"

#include "framework/framework.h"

constexpr int ring_size = 200;

struct LoadedImg {
    Image img;
    int page_num;
    int chap_id;
};

struct Scan {
    Texture tex;
    ImVec2 size;
    float mul = 1.f;
    int page_num;
    int chap_id;
};

struct Chapter {
    int number;
    int length;
};

struct Reader {
    void init();
    void close();
    void load_images(int chapter);
    void frame();

    std::atomic<int> loaded_count;
    std::atomic<int> pages_count;
    std::atomic<bool> should_stop = false;
    std::atomic<bool> still_loading = false;

    int cur_scan = 0;
    bool need_to_load = false;
    int chap_to_load = 0;

    vec<LoadedImg> images;
    vec<Scan> scans;
    vec<Chapter> chapters;

    ImVec2 offset;
};

extern Reader reader;