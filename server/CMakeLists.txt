

add_executable(server main.c server.c cli.c)

target_include_directories(server PUBLIC "./include" "../c2-lib/include")

add_dependencies(server c2_lib)

target_link_libraries(server PUBLIC c2_lib)