cmake_minimum_required(VERSION 3.20)

project(UKFStockPredictor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(external/IXWebSocket)

set(GLFW_ROOT "${CMAKE_SOURCE_DIR}/external/glfw-3.4.bin.WIN64")

add_executable(UKFStockPredictor
    src/main.cpp
    src/Research.cpp
    src/unscented_kalman_filter.cpp
    src/UI.cpp

    external/glad/src/gl.c

    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/imgui/imgui_demo.cpp
    external/implot/implot.cpp
    external/implot/implot_items.cpp
)

target_include_directories(UKFStockPredictor PRIVATE
    include
    libs

    external/glad/include

    external/imgui
    external/imgui/backends
    external/implot

    ${GLFW_ROOT}/include
    external/json/single_include
)

target_link_directories(UKFStockPredictor PRIVATE
    ${GLFW_ROOT}/lib-vc2022
)

target_link_libraries(UKFStockPredictor PRIVATE
    glfw3
    opengl32
    ixwebsocket::ixwebsocket
)

add_custom_command(TARGET UKFStockPredictor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GLFW_ROOT}/lib-vc2022/glfw3.dll"
        $<TARGET_FILE_DIR:UKFStockPredictor>
    COMMENT "Copying glfw3.dll next to UKFStockPredictor"
)
