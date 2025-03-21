#include "Input.h"
#include <GLFW/glfw3.h>

float posX = 0.0f, posY = 0.0f;
const float speed = 0.05f;

void Input::process(Window& window) {
    GLFWwindow* win = window.getGLFWwindow();
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) posY += speed;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) posY -= speed;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) posX -= speed;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) posX += speed;
}
