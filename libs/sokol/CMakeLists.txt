# add_library(sokol sokol.cc)

# # SOKOL_GLCORE33, SOKOL_D3D11, SOKOL_METAL
# option(SOKOL_BACKEND "Select 3D backend API" SOKOL_GLCORE33)
# option(USE_ARC "Enable/disable ARC" OFF)

# if (CMAKE_SYSTEM_NAME STREQUAL Linux)
#     set(SOKOL_BACKEND SOKOL_GLCORE33)
# elseif (CMAKE_SYSTEM_NAME STREQUAL Darwin)
#     set(SOKOL_BACKEND SOKOL_METAL)
# elseif (CMAKE_SYSTEM_NAME STREQUAL Windows)
#     set(SOKOL_BACKEND SOKOL_D3D11)
# else()
#     message(FATAL_ERROR "Unrecognized CMAKE_SYSTEM_NAME")
# endif()

# message(">> SOKOL_BACKEND: ${SOKOL_BACKEND}")

# add_compile_definitions(${SOKOL_BACKEND})

# target_include_directories(sokol PRIVATE ../imgui)
# target_include_directories(sokol PUBLIC .)

fips_begin_lib(sokol)
    fips_src(.)
    fips_deps(imgui)
fips_end_lib(sokol)

target_compile_definitions(sokol PRIVATE ${SOKOL_BACKEND})
target_include_directories(sokol PUBLIC .)
