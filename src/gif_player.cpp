#include "gif_player.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>

// GL 1.2+ enumerants missing from the stock MinGW GL 1.1 header.
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "imgui.h"

#include <cstdio>
#include <cstdlib>
#include <cstdint>

GifPlayer::~GifPlayer() {
    // Context is normally already gone here; shutdown() clears explicitly
    // while the context is still current. This is just a safety net.
    frames_.clear();
}

void GifPlayer::clear() {
    for (auto& f : frames_) {
        if (f.tex != 0)
            glDeleteTextures(1, reinterpret_cast<GLuint*>(&f.tex));
        f.tex = 0;
    }
    frames_.clear();
    w_ = h_ = 0;
    accMs_ = 0;
    index_ = 0;
}

bool GifPlayer::load(const std::string& path) {
    clear();

    FILE* fp = nullptr;
    fopen_s(&fp, path.c_str(), "rb");
    if (!fp)
        return false;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size <= 0) {
        fclose(fp);
        return false;
    }
    std::vector<uint8_t> data(static_cast<size_t>(size));
    size_t got = fread(data.data(), 1, data.size(), fp);
    fclose(fp);
    if (got != data.size())
        return false;

    int* delays = nullptr;
    int w = 0, h = 0, n = 0, comp = 0;
    // Key technical note: whole file -> memory -> stbi_load_gif_from_memory.
    uint8_t* pixels = stbi_load_gif_from_memory(data.data(),
                                                static_cast<int>(data.size()),
                                                &delays, &w, &h, &n, &comp, 4);
    if (!pixels || n <= 0 || w <= 0 || h <= 0) {
        if (pixels) stbi_image_free(pixels);
        return false;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    const size_t frameBytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
    for (int i = 0; i < n; ++i) {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels + i * frameBytes);
        int delay = (delays != nullptr) ? delays[i] : 100;  // stb: milliseconds
        if (delay < 20)
            delay = 100;  // GIF "0 delay" convention -> ~10 fps
        frames_.push_back(Frame{ static_cast<unsigned int>(tex), delay });
    }
    stbi_image_free(pixels);
    if (delays)
        stbi_image_free(delays);

    w_ = w;
    h_ = h;
    return !frames_.empty();
}

void GifPlayer::update(uint64_t deltaMs) {
    if (frames_.empty())
        return;
    accMs_ += deltaMs;
    // Safety counter: never spin forever even on pathological delays.
    for (size_t guard = 0; guard < frames_.size() + 1; ++guard) {
        int delay = frames_[index_].delayMs;
        if (delay <= 0 || accMs_ < static_cast<uint64_t>(delay))
            break;
        accMs_ -= static_cast<uint64_t>(delay);
        index_ = (index_ + 1) % frames_.size();
    }
}

unsigned int GifPlayer::texture() const {
    if (frames_.empty())
        return 0;
    return frames_[index_].tex;
}

void GifPlayer::draw(float sizePx) const {
    unsigned int tex = texture();
    if (tex == 0)
        return;
    ImGui::Image(static_cast<ImTextureID>(static_cast<intptr_t>(tex)),
                 ImVec2(sizePx, sizePx));
}
