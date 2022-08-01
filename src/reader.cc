#include "reader.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define IMGUI_DEFINE_MATH_OPERATORS 
#include "imgui_internal.h"

#include <sokol_app.h>

#include "utils/http.h"

#include <sokol_fetch.h>

#include "app.h"
#include "tracelog.h"
#include "utils/utils.h"

Reader reader;

static void init_dock();
static void show_page_num(int page_num, int chap_num, bool *p_open = nullptr);

void Reader::init() {
    int chap = 1;

    str chap_str = read_whole_file("cache/last_chap.txt");
    if (!chap_str.empty()) {
        chap = atoi(chap_str.buf);
        info("reading from chap %d", chap);
    }
    else {
        info("couldn't find last_chap.txt, defaulting to %d", chap);
    }

    load_images(chap);
}

void Reader::close() {
    int chap_id = scans[cur_scan].chap_id;
    int chap = chapters[chap_id].number;

    info("saving chap %d", chap);

    FILE *fp = fopen("cache/last_chap.txt", "wb");
    fputs(format("%d", chap), fp);
    fclose(fp);
}

void Reader::load_images(int chapter) {
    // check that we didn't already load the chapter
    for (auto &chap : chapters) {
        if (chap.number == chapter) {
            warn("already loaded chapter %d", chapter);
            return;
        }
    }

    chapters.append({ chapter, 0 });

    still_loading = true;

    std::thread load(
        [this, chapter](){
            info("loading images from chapter: %d", chapter);
            system(format("python3 get_images.py %d", chapter));
            vec<str_view> images_url = split_lines(
                read_whole_file(format("cache/chap-%d.txt", chapter))
            );

            int page_num = 0;
            int chap_id = chapters.len - 1;

            images.resize(images.len + images_url.len);
            pages_count = (int)images.len;
            chapters[chap_id].length = images_url.len;

            for (const auto &img : images_url) {
                if (should_stop) {
                    should_stop = false;
                    still_loading = false;
                    return;
                }
                
                str_view hostname = img.sub(8, 25);
                str_view uri = img.sub(25);

                auto res = http::get(hostname, uri).unpack();
                if (res.status != http::STATUS_OK) {
                    err("req failed: %d", (int)res.status);
                    err("data:\n%s", res.data.buf);
                }
                else {
                    images[loaded_count++] = {
                        loadImageFromMemory(res.data.buf, res.data.len),
                        ++page_num,
                        chap_id
                    };
                }
            }

            info("finished loading images from chapter: %d", chapter);
            still_loading = false;
        }
    );
    load.detach();
}

void Reader::frame() {
    int local_count = loaded_count;
    while (scans.len < local_count) {
        usize i = scans.len;
        auto tex = loadTextureFromImage(images[i].img);
        ImVec2 sz = { (f32)images[i].img.width, (f32)images[i].img.height };
        scans.append({tex, sz, 1.f, images[i].page_num, images[i].chap_id});
        freeImage(images[i].img);
    }

    for (int i = cur_scan - 30; i >= 0; --i) {
        auto &s = scans[i];
        if (s.tex.id != 0) {
            freeTexture(s.tex);
            s.tex.id = 0;
        }
    }

    if (need_to_load && !still_loading) {
        need_to_load = false;
        load_images(chap_to_load);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
        warn("fullscreen");
        sapp_toggle_fullscreen();
    }

    static bool show_chap_select = false;
    if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
        show_chap_select = true;
    }
    if (show_chap_select) {
        static int chap = 1;

        ImGui::Begin("Chapter Select", &show_chap_select);
            ImGui::InputInt("Chapter", &chap, 0);
            if (ImGui::Button("Go")) {
                chap_to_load = chap;
                need_to_load = true;
                should_stop = true;
                cur_scan = loaded_count;
            }
        ImGui::End();
    }

    init_dock();

    if (cur_scan >= scans.len) return;

    ImGui::Begin("Reader", nullptr, ImGuiWindowFlags_NoScrollWithMouse);
        auto &scan = scans[cur_scan];
        if (scan.tex.id != 0) {
            auto id = (ImTextureID)((uintptr_t)scan.tex.id);
            ImVec2 size = scan.size;
            ImVec2 winsize = ImGui::GetWindowSize();

            // TODO not great but works decently
            if (size.x > winsize.x) {
                size.y = winsize.x * size.y / size.x;
                size.x = winsize.x;
            }
            else {
                size.x = winsize.y * size.x / size.y;
                size.y = winsize.y;
            }

            size *= scan.mul;
            ImGui::SetCursorPos((winsize - size) * 0.5f + offset);
            ImGui::Image(id, size);
        }
    ImGui::End();

    static bool show_pagenum = true;
    show_page_num(
        scans[cur_scan].page_num, 
        chapters[scans[cur_scan].chap_id].number, 
        &show_pagenum
    );
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false)) {
        show_pagenum = true;
    }

    auto was_scan = cur_scan;

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
        cur_scan = max(cur_scan - 1, 0);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
        cur_scan = min(cur_scan + 1, scans.len - 1);
        if (!still_loading) {
            auto &scan = scans[cur_scan];
            auto &chap = chapters[scan.chap_id];

            // if theres only 8 pages left to read
            if (scan.page_num >= (chap.length - 8)) {
                chap_to_load = chap.number + 1;
                need_to_load = true;
            }
        }
    }

    if (was_scan != cur_scan) {
        scans[was_scan].mul = 1.f;
        offset = { 0, 0 };
    }

    if (ImGui::IsKeyPressed(ImGuiKey_RightBracket, false)) {
        scans[cur_scan].mul *= 1.2f;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Slash, false)) {
        scans[cur_scan].mul *= 0.8f;
    }

    static ImVec2 start_off = { 0, 0 };
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        offset = start_off + ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
    }
    else {
        start_off = offset;
    }
}

static void init_dock() {
    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("Reader");
        ImGui::DockSpace(dockspace_id, { 0, 0 }, ImGuiDockNodeFlags_NoTabBar);

        static bool first_time = true;
        if (first_time) {
            first_time = false;

            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            // we now dock our windows into the docking node we made above
            ImGui::DockBuilderDockWindow("Reader", dockspace_id);
            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    ImGui::End();
}

static void show_page_num(int page_num, int chap_num, bool *p_open) {
    static int corner = 0;
    auto &io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (corner != -1) {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowViewport(viewport->ID);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Page Count", p_open, window_flags)) {
        ImGui::Text("Page: %d", page_num);
        ImGui::Text("Chapter: %d", chap_num);

        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Custom",       NULL, corner == -1)) corner = -1;
            if (ImGui::MenuItem("Top-left",     NULL, corner == 0)) corner = 0;
            if (ImGui::MenuItem("Top-right",    NULL, corner == 1)) corner = 1;
            if (ImGui::MenuItem("Bottom-left",  NULL, corner == 2)) corner = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
            if (p_open && ImGui::MenuItem("Close")) *p_open = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}