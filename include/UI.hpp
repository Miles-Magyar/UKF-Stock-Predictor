#include <iostream>
#include <vector>
#include <string>

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "implot.h"

class UI {
public:
    void run();

    void updateData(
        const std::vector<double>& timestamps,
        const std::vector<double>& slope,
        const std::vector<double>& spread
    );

private:
    std::vector<double> timestamps;
    std::vector<double> slope;
    std::vector<double> spread;
};
