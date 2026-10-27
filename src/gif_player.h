#pragma once

#include <cstdint>
#include <string>
#include <vector>

// M3 — Animated GIF renderer ------------------------------------------------
// Loads every frame of a GIF via stb_image into one GL texture per frame.
// Requires a current OpenGL context for load()/clear().
class GifPlayer {
public:
    ~GifPlayer();

    bool load(const std::string& path);
    void update(uint64_t deltaMs);
    void reset() { accMs_ = 0; index_ = 0; }
    void clear();

    // OpenGL texture handle of the currently visible frame (0 if none).
    unsigned int texture() const;
    bool   valid() const { return !frames_.empty(); }
    int    width() const { return w_; }
    int    height() const { return h_; }
    size_t frameCount() const { return frames_.size(); }
    void   draw(float sizePx) const;  // ImGui::Image at cursor position

private:
    struct Frame {
        unsigned int tex = 0;  // GLuint
        int delayMs = 100;
    };
    std::vector<Frame> frames_;
    int      w_ = 0, h_ = 0;
    uint64_t accMs_ = 0;
    size_t   index_ = 0;
};
