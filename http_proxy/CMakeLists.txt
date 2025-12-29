cmake_minimum_required(VERSION 3.10.0)
project(proxy VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(proxy 
    client_handler.c 
    http_parser.c 
    main.c 
    network_utils.c 
    proxy_server.c
)

target_compile_options(proxy PRIVATE
    -Werror
    -pedantic
    -Wextra
    -O2
)

target_link_libraries(proxy pthread)