#include "app.h"

int main() {
    const int screen_width = 1280;
    const int screen_height = 720;

    std::printf("Hello\n");
    App app = App("Simulator", screen_width, screen_height);
    app.run();

    return 0;
}
