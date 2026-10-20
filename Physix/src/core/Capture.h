#pragma once
#include <cstdint>
#include <string>

// Screenshot/GIF capture used by Application.cpp's CLI benchmark/capture mode
// (--screenshot-at, --gif) to produce the README demo assets in
// tools/capture_demo.ps1, without depending on an external tool (ffmpeg,
// Python) that this environment doesn't reliably have. Both read directly
// from the currently-bound default framebuffer via glReadPixels, so they work
// the same whether the window is visible or created with --hidden.
//
// Backed by two vendored single-header libraries (Dependencies/stb,
// Dependencies/gif), the same header-only vendoring pattern already used for
// GLM.
namespace Capture {
    // Reads the current framebuffer (width x height pixels) and writes it as
    // a PNG to `path`. Returns false on failure (bad path, glReadPixels
    // error, etc).
    bool SaveScreenshotPNG(const std::string& path, int width, int height);
}

// Wraps gif.h's Begin/WriteFrame/End sequence to accumulate framebuffer
// captures into one animated GIF. Not copyable -- owns a live file handle
// (via the vendored GifWriter) between Begin() and End().
class GifRecorder {
public:
    GifRecorder() = default;
    ~GifRecorder();

    GifRecorder(const GifRecorder&) = delete;
    GifRecorder& operator=(const GifRecorder&) = delete;

    // Opens `path` and writes the GIF header. fps controls the per-frame
    // delay (converted to gif.h's centisecond units). Returns false on
    // failure; safe to call again after a failed Begin.
    bool Begin(const std::string& path, int width, int height, int fps);

    // Grabs the current framebuffer as the next frame. No-op (returns false)
    // if Begin() wasn't called or already failed.
    bool CaptureFrame();

    // Closes the file. Safe to call even if Begin() was never called.
    void End();

    bool IsRecording() const { return recording; }

private:
    struct Impl; // hides gif.h's GifWriter from this header
    Impl* impl = nullptr;
    int width = 0;
    int height = 0;
    int delayCentiseconds = 4;
    bool recording = false;
};
