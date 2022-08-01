fips_begin_lib(colla)
    fips_src(.)
fips_end_lib()

if(MSVC)
    target_link_libraries(colla ws2_32.lib)
    target_compile_options(colla PRIVATE /W4)
else()
    target_link_libraries(colla pthread)
    target_compile_options(colla PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_definitions(colla PUBLIC _DEFAULT_SOURCE)
endif()

target_include_directories(colla PUBLIC .)