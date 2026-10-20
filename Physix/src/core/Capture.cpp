// See Capture.h. Owns the two vendored single-header library
// implementations (STB_IMAGE_WRITE_IMPLEMENTATION / gif.h's inline
// functions) -- this is the one translation unit that defines them.
//
// stb_image_write.h calls plain sprintf() internally (for the tempfile-name
// path stbi_write_tga uses, unrelated to the PNG path this file actually
// calls) -- MSVC's deprecation warning for it is scoped to just this
// translation unit rather than disabled project-wide. Must be defined before
// any CRT header is pulled in transitively, hence first line of the file.
#define _CRT_SECURE_NO_WARNINGS

#include "Capture.h"
#include <glad/glad.h>
#include <vector>
#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// OpenGL's framebuffer origin is bottom-left; gif.h's flip flag corrects for
// that so the recorded GIF isn't upside down.
#define GIF_FLIP_VERT
#include <gif.h>

namespace {
    // Both PNG and GIF capture need the same RGBA8 framebuffer read -- shared
    // here so SaveScreenshotPNG and GifRecorder::CaptureFrame don't duplicate
    // the glReadPixels call and buffer sizing.
    std::vector<uint8_t> ReadFramebufferRGBA(int width, int height) {
        std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        return pixels;
    }
}

bool Capture::SaveScreenshotPNG(const std::string& path, int width, int height) {
    if (width <= 0 || height <= 0) return false;
    std::vector<uint8_t> pixels = ReadFramebufferRGBA(width, height);
    // stbi_write_png expects a top-down image; OpenGL's readback is
    // bottom-up, so flip via a negative stride rather than reordering rows
    // ourselves.
    int stride = width * 4;
    const uint8_t* lastRow = pixels.data() + static_cast<size_t>(height - 1) * stride;
    return stbi_write_png(path.c_str(), width, height, 4, lastRow, -stride) != 0;
}

struct GifRecorder::Impl {
    GifWriter writer{};
};

GifRecorder::~GifRecorder() {
    End();
}

bool GifRecorder::Begin(const std::string& path, int w, int h, int fps) {
    End(); // in case Begin() is called twice without an intervening End()

    if (w <= 0 || h <= 0 || fps <= 0) return false;

    impl = new Impl();
    width = w;
    height = h;
    // gif.h's delay is in hundredths of a second per frame.
    delayCentiseconds = (fps > 0) ? std::max(1, 100 / fps) : 4;

    recording = GifBegin(&impl->writer, path.c_str(),
        static_cast<uint32_t>(width), static_cast<uint32_t>(height),
        static_cast<uint32_t>(delayCentiseconds));
    if (!recording) {
        delete impl;
        impl = nullptr;
    }
    return recording;
}

bool GifRecorder::CaptureFrame() {
    if (!recording || !impl) return false;
    std::vector<uint8_t> pixels = ReadFramebufferRGBA(width, height);
    return GifWriteFrame(&impl->writer, pixels.data(),
        static_cast<uint32_t>(width), static_cast<uint32_t>(height),
        static_cast<uint32_t>(delayCentiseconds));
}

void GifRecorder::End() {
    if (recording && impl) {
        GifEnd(&impl->writer);
    }
    delete impl;
    impl = nullptr;
    recording = false;
}
