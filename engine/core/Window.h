#pragma once
#include <GLFW/glfw3.h>

class Window {
public:
    Window(int width, int height, const char* title);
    bool init();
    void update();
    bool shouldClose();
    void cleanup();
    GLFWwindow* getGLFWwindow();

private:
    int width, height;
    const char* title;
    GLFWwindow* window;
};
