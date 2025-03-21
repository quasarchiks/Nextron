#include "Window.h"
#include <iostream>
#include <glad/glad.h>

Window::Window(int w, int h, const char* t) : width(w), height(h), title(t), window(nullptr) {}

bool Window::init() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    glViewport(0, 0, width, height);
    return true;
}

void Window::update() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool Window::shouldClose() { return glfwWindowShouldClose(window); }

void Window::cleanup() {
    glfwTerminate();
}

GLFWwindow* Window::getGLFWwindow() { return window; }
