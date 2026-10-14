# Keep this file source-only so clang-tidy can scope manifest changes safely.

# 1. Automatically grab all .cpp files in the app folder and its subdirectories
file(
    GLOB_RECURSE PLATFORMER_SOURCES
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/app/*.cpp"
    "${PROJECT_SOURCE_DIR}/app/debug/*.cpp"
    "${PROJECT_SOURCE_DIR}/app/content/*.cpp"
    "${PROJECT_SOURCE_DIR}/app/game/*.cpp"
    "${PROJECT_SOURCE_DIR}/app/graphics/*.cpp"
    "${PROJECT_SOURCE_DIR}/app/ui/*.cpp"
)

# 2. Tell Visual Studio to strip the base path and mirror the disk layout
source_group(TREE "${PROJECT_SOURCE_DIR}/app" PREFIX "app" FILES ${PLATFORMER_SOURCES})

# 3. Add the dynamically discovered files to your target
target_sources(
    simple_platformer
    PRIVATE
    ${PLATFORMER_SOURCES}
)
