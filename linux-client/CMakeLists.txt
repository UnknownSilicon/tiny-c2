
add_executable(linux-client main.c caps.c)

set(CAPS_LIST_CMAKE "CAP_SYSTEM")

configure_file("include/caps.h.in" ${CMAKE_BINARY_DIR}/generated/caps.h)

target_include_directories(linux-client PUBLIC "./include" "../c2-lib/include" ${CMAKE_BINARY_DIR}/generated/)

add_dependencies(linux-client c2_lib)

target_link_libraries(linux-client PUBLIC c2_lib)