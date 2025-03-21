#include <iostream>
#include "engine/core/Window.h"
#include "engine/graphics/Renderer.h"
#include "engine/input/Input.h"

int main() {
    Window window(800, 600, "Nextron");
    if (!window.init()) return -1;

    Renderer renderer;
    renderer.init();

    while (!window.shouldClose()) {
        Input::process(window);
        renderer.render();
        window.update();
    }

    window.cleanup();
    return 0;
}
