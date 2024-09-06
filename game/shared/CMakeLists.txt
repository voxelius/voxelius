add_library(shared STATIC
    "${CMAKE_CURRENT_LIST_DIR}/entity/transform.cc"
    "${CMAKE_CURRENT_LIST_DIR}/entity/velocity.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk.cc"
    "${CMAKE_CURRENT_LIST_DIR}/chunk_coord.cc"
    "${CMAKE_CURRENT_LIST_DIR}/game_voxels.cc"
    "${CMAKE_CURRENT_LIST_DIR}/globals.cc"
    "${CMAKE_CURRENT_LIST_DIR}/local_coord.cc"
    "${CMAKE_CURRENT_LIST_DIR}/protocol.cc"
    "${CMAKE_CURRENT_LIST_DIR}/ray_dda.cc"
    "${CMAKE_CURRENT_LIST_DIR}/splash.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vdef.cc"
    "${CMAKE_CURRENT_LIST_DIR}/vgen.cc"
    "${CMAKE_CURRENT_LIST_DIR}/voxel_coord.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world.cc"
    "${CMAKE_CURRENT_LIST_DIR}/world_coord.cc")
target_include_directories(shared PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(shared PUBLIC common emhash enet entt FNL miniz parson thread_pool)
