// M3 self-test: verify the shipped GIFs decode via stb_image exactly like
// GifPlayer does (frames + per-frame delays). Not part of the app build.
#include <cstdio>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static int check(const char* path) {
    FILE* fp = nullptr;
    fopen_s(&fp, path, "rb");
    if (!fp) { std::printf("%s: CANNOT OPEN\n", path); return 1; }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::vector<unsigned char> data((size_t)size);
    if (fread(data.data(), 1, data.size(), fp) != data.size()) {
        fclose(fp);
        std::printf("%s: READ ERROR\n", path);
        return 1;
    }
    fclose(fp);
    int* delays = nullptr;
    int w = 0, h = 0, n = 0, comp = 0;
    unsigned char* px = stbi_load_gif_from_memory(
        data.data(), (int)data.size(), &delays, &w, &h, &n, &comp, 4);
    if (!px || n <= 0) {
        std::printf("%s: DECODE FAILED (%s)\n", path, stbi_failure_reason());
        return 1;
    }
    std::printf("%s: OK %dx%d %d frames comp=%d delay0=%dms\n", path, w, h, n,
                comp, delays ? delays[0] : -1);
    stbi_image_free(px);
    if (delays) stbi_image_free(delays);
    return (w > 300 || h > 300) ? 2 : 0;  // spec: GIFs <= 300x300
}
int main() {
    int rc = 0;
    rc |= check("C:/Users/MSIRAI~1/PomodoroApp/assets/rocket.gif");
    rc |= check("C:/Users/MSIRAI~1/PomodoroApp/assets/clouds.gif");
    rc |= check("C:/Users/MSIRAI~1/PomodoroApp/assets/beach.gif");
    return rc;
}
