

add_executable(server main.c server.c cli.c msg_reader.c handler.c ipc.c)

target_include_directories(server PUBLIC "./include" "../c2-lib/include")

add_dependencies(server c2_lib)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(server PUBLIC c2_lib Threads::Threads)