#
# project: jojo-reader
#
cmake_minimum_required(VERSION 2.8)
project(jojo-reader)
# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

fips_setup()
fips_project(jojo-reader)

if (FIPS_EMSCRIPTEN)
    set(SOKOL_BACKEND SOKOL_GLES3)
    set(SLANG "glsl300es:glsl100")
elseif (FIPS_ANDROID)
    set(SOKOL_BACKEND SOKOL_GLES3)
    set(SLANG "glsl300es:glsl100")
elseif (FIPS_WINDOWS)
    set(SOKOL_BACKEND SOKOL_D3D11)
    set(SLANG "hlsl4")
elseif (FIPS_MACOS)
    set(SOKOL_BACKEND SOKOL_METAL)
    set(SLANG "metal_macos")
elseif (FIPS_IOS)
    set(SOKOL_BACKEND SOKOL_METAL)
    set(SLANG "metal_ios:metal_sim")
else()
    set(SOKOL_BACKEND SOKOL_GLCORE33)
    set(SLANG "glsl330")
endif()

fips_add_subdirectory(libs)

fips_begin_app(jojo-reader windowed)
    fipsutil_copy(assets.yml)
    fips_dir(src)
        fips_files(
            app.h app.cc
            reader.h reader.cc
            tracelog.h tracelog.c
            main.cc
        )
    fips_dir(src/framework)
        fips_files(
            framework.h framework.c
        )
    fips_dir(src/utils)
        fips_files(
            defines.h 
            http.h http.cc
            map.h
            move.h
            optional.h
            print.h print.cc
            rune.h 
            slice.h
            str.h str.cc
            utils.h utils.cc
            vec.h
            xmalloc.h xmalloc.cc
        )
    fips_dir(shaders)
        sokol_shader(base.glsl ${SLANG})
    fips_deps(imgui sokol stb colla)
fips_end_app()

target_include_directories(jojo-reader PRIVATE src/framework)

fips_finish()
