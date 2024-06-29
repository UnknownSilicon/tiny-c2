
add_library(c2_lib tc2.c aes.c util.c)

target_include_directories(c2_lib PUBLIC "./include")