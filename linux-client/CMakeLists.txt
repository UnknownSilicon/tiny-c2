

add_executable(linux-client main.c messaging.c)

target_include_directories(linux-client PUBLIC "./include" "../c2-lib/include")

add_dependencies(linux-client c2_lib)

target_link_libraries(linux-client PUBLIC c2_lib)