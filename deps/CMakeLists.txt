find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_subdirectory(ed25519)
add_subdirectory(eigen)
add_subdirectory(emhash)
add_subdirectory(enet)
add_subdirectory(entt)
add_subdirectory(fastnoiselite)
add_subdirectory(glad)
add_subdirectory(imgui)
add_subdirectory(lua)
add_subdirectory(miniz)
add_subdirectory(parson)
add_subdirectory(physfs)
add_subdirectory(SDL3)
add_subdirectory(stb)
add_subdirectory(thread_pool)
add_subdirectory(tracy)

# Generate thirdparty.txt; I find it annoying to manually aggregate
# all the third-party libraries licenses from all over the place so
# this convenient script is going to do that for me automatically
add_custom_target(thirdparty_txt ALL
    COMMAND ${Python3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/scripts/generate-txt-thirdparty.py" "${PROJECT_SOURCE_DIR}/deps"
    BYPRODUCTS "${PROJECT_BINARY_DIR}/thirdparty.txt"
    DEPENDS "${PROJECT_SOURCE_DIR}/scripts/generate-txt-thirdparty.py"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
install(FILES "${PROJECT_BINARY_DIR}/thirdparty.txt" DESTINATION ".")
