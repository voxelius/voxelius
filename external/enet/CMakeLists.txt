cmake_minimum_required(VERSION 3.15)
project(enet LANGUAGES C VERSION 1.3.17)

add_library(enet STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/callbacks.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/compress.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/host.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/list.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/packet.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/peer.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/protocol.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/unix.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/win32.c")
target_include_directories(enet PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
if(WIN32)
    target_compile_definitions(enet PRIVATE _WINSOCK_DEPRECATED_NO_WARNINGS)
    target_compile_options(enet PRIVATE -W3)
    target_link_libraries(enet INTERFACE winmm ws2_32)
else()
    target_compile_options(enet PRIVATE -Wno-error)
endif()

include(CheckFunctionExists)
include(CheckStructHasMember)
include(CheckTypeSize)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/types.h" "sys/socket.h")
check_function_exists("fcntl" HAS_FCNTL)
check_function_exists("poll" HAS_POLL)
check_function_exists("getaddrinfo" HAS_GETADDRINFO)
check_function_exists("getnameinfo" HAS_GETNAMEINFO)
check_function_exists("gethostbyname_r" HAS_GETHOSTBYNAME_R)
check_function_exists("gethostbyaddr_r" HAS_GETHOSTBYADDR_R)
check_function_exists("inet_pton" HAS_INET_PTON)
check_function_exists("inet_ntop" HAS_INET_NTOP)
check_struct_has_member("struct msghdr" "msg_flags" "sys/types.h;sys/socket.h" HAS_MSGHDR_FLAGS)
check_type_size("socklen_t" HAS_SOCKLEN_T BUILTIN_TYPES_ONLY)

if(HAS_FCNTL)
    target_compile_definitions(enet PRIVATE HAS_FCNTL)
endif()

if(HAS_POLL)
    target_compile_definitions(enet PRIVATE HAS_POLL)
endif()

if(HAS_GETNAMEINFO)
    target_compile_definitions(enet PRIVATE HAS_GETNAMEINFO)
endif()

if(HAS_GETADDRINFO)
    target_compile_definitions(enet PRIVATE HAS_GETADDRINFO)
endif()

if(HAS_GETHOSTBYNAME_R)
    target_compile_definitions(enet PRIVATE HAS_GETHOSTBYNAME_R)
endif()

if(HAS_GETHOSTBYADDR_R)
    target_compile_definitions(enet PRIVATE HAS_GETHOSTBYADDR_R)
endif()

if(HAS_INET_PTON)
    target_compile_definitions(enet PRIVATE HAS_INET_PTON)
endif()

if(HAS_INET_NTOP)
    target_compile_definitions(enet PRIVATE HAS_INET_NTOP)
endif()

if(HAS_MSGHDR_FLAGS)
    target_compile_definitions(enet PRIVATE HAS_MSGHDR_FLAGS)
endif()

if(HAS_SOCKLEN_T)
    target_compile_definitions(enet PRIVATE HAS_SOCKLEN_T)
endif()
