cmake_minimum_required(VERSION 3.20)


add_executable(pathsearch
    "sources/main.cpp"
    "sources/dungeon/dungeonGenerator.cpp"
    "sources/dungeon/dungeonUtils.cpp"
    "sources/dungeon/pathsearch.cpp"
)
target_include_directories(pathsearch PRIVATE "sources")
target_link_libraries(pathsearch
    fmt spdlog function2 glm::glm allegro mdspan stdgenerator
    allegro_font allegro_image allegro_primitives DearImGui)
target_compile_definitions(pathsearch PRIVATE "PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"")

copy_allegro_dlls(pathsearch)
