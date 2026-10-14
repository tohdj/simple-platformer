# Keep this file source-only so clang-tidy can scope manifest changes safely.

# 1. Automatically grab all .cpp files in the app folder and its subdirectories
file(
    GLOB_RECURSE PLATFORMER_CORE_SOURCES
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/actor/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/combat/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/input/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/inventory/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/math/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/movement/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/navigation/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/npc/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/physics/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/render/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/timing/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/world/*.cpp"
)

# 2. Tell Visual Studio to strip the base path and mirror the disk layout
source_group(TREE "${PROJECT_SOURCE_DIR}/src" PREFIX "src" FILES ${PLATFORMER_CORE_SOURCES})

# 3. Add the dynamically discovered files to your target
target_sources(
    simple_platformer_core
    PRIVATE
    ${PLATFORMER_CORE_SOURCES}
)





# Keep this file source-only so clang-tidy can scope manifest changes safely.
target_sources(
    simple_platformer_core
    PRIVATE
)
