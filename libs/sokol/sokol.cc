#define SOKOL_IMPL

extern void __sokol_log(const char *msg);
#define SOKOL_LOG(msg) __sokol_log(msg)

#include <imgui.h>
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_imgui.h"
#include "sokol_time.h"
#include "sokol_fetch.h"