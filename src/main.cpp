#include "app.h"

// SDL2main (linked on both MSVC and MinGW) provides WinMain and calls here,
// so the app runs with no console window (M7: WIN32_EXECUTABLE).
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    App app;
    if (!app.init())
        return 1;
    return app.run();
}
