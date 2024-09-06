cmake_minimum_required(VERSION 3.15)
project(PhysFS VERSION 3.3.0 LANGUAGES C)
add_library(physfs STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_byteorder.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_unicode.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_platform_posix.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_platform_unix.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_platform_windows.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_platform_os2.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_platform_qnx.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_platform_android.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_dir.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_unpacked.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_grp.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_hog.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_7z.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_mvl.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_qpak.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_wad.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_zip.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_slb.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_iso9660.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/physfs_archiver_vdf.c")
target_include_directories(physfs PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

if(APPLE)
    target_link_libraries(physfs PRIVATE "-framework IOkit -framework Foundation")
    target_sources(physfs PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/physfs_platform_apple.m")
endif()

if(UNIX AND NOT WIN32 AND NOT APPLE)
    find_library(PTHREAD_LIBRARY pthread)
    if(PTHREAD_LIBRARY)
        target_link_libraries(physfs PRIVATE ${PTHREAD_LIBRARY})
    endif()
endif()

option(PHYSFS_ARCHIVE_ZIP "Enable ZIP support" TRUE)
if(NOT PHYSFS_ARCHIVE_ZIP)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_ZIP=0)
endif()

option(PHYSFS_ARCHIVE_7Z "Enable 7zip support" TRUE)
if(NOT PHYSFS_ARCHIVE_7Z)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_7Z=0)
endif()

option(PHYSFS_ARCHIVE_GRP "Enable Build Engine GRP support" TRUE)
if(NOT PHYSFS_ARCHIVE_GRP)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_GRP=0)
endif()

option(PHYSFS_ARCHIVE_WAD "Enable Doom WAD support" TRUE)
if(NOT PHYSFS_ARCHIVE_WAD)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_WAD=0)
endif()

option(PHYSFS_ARCHIVE_HOG "Enable Descent I/II HOG support" TRUE)
if(NOT PHYSFS_ARCHIVE_HOG)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_HOG=0)
endif()

option(PHYSFS_ARCHIVE_MVL "Enable Descent I/II MVL support" TRUE)
if(NOT PHYSFS_ARCHIVE_MVL)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_MVL=0)
endif()

option(PHYSFS_ARCHIVE_QPAK "Enable Quake I/II QPAK support" TRUE)
if(NOT PHYSFS_ARCHIVE_QPAK)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_QPAK=0)
endif()

option(PHYSFS_ARCHIVE_SLB "Enable I-War / Independence War SLB support" TRUE)
if(NOT PHYSFS_ARCHIVE_SLB)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_SLB=0)
endif()

option(PHYSFS_ARCHIVE_ISO9660 "Enable ISO9660 support" TRUE)
if(NOT PHYSFS_ARCHIVE_ISO9660)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_ISO9660=0)
endif()

option(PHYSFS_ARCHIVE_VDF "Enable Gothic I/II VDF archive support" TRUE)
if(NOT PHYSFS_ARCHIVE_VDF)
    target_compile_definitions(physfs PUBLIC PHYSFS_SUPPORTS_VDF=0)
endif()
