// Application.cpp: composition root and per-frame game loop. Creates the
// Window, Shader, Camera, mesh objects (shared by non-owning pointer across
// many Sphere/Cuboid instances), a Scene (owns the dynamic
// spheres + static walls and spawns new spheres over time), and a
// PhysicsWorld (owns the collision/integration pipeline). Each frame: spawn,
// clear, read input, sort spheres back-to-front for alpha blending, draw
// spheres then walls, then advance physics by one frame via
// PhysicsWorld::Step -- physics runs *after* rendering so the frame just
// drawn used the previous frame's resolved positions. No physics or
// collision logic lives in this file; see physics/PhysicsWorld.h and
// physics/RigidBody.h for that.
#include <glad/glad.h>      // Must be included before GLFW
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <vector>
#include <cmath>
#include <random>
#include <thread>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "sphere_mesh.h"
#include "sphere.h"
#include "shader.h"
#include "Cuboid_mesh.h"  // For the floor
#include "Cuboid.h"       // For the floor
#include "camera.h"
#include "physics/PhysicsWorld.h"
#include "scene/Scene.h"
#include "core/Window.h"
#include "core/Input.h"
#include "interaction/Picker.h"
#include "interaction/Selection.h"
#include "interaction/VelocityTool.h"
#include "core/DebugUI.h"
#include "core/FrameStats.h"
#include "core/Capture.h"
#include <imgui.h>

Sphere_mesh get_sphere_mesh(float radius, int sectors, int stacks) {
   return Sphere_mesh(radius, sectors, stacks);
}

Cuboid_mesh get_cuboid_mesh(float height, float width, float thickness) {
    return Cuboid_mesh(height, width, thickness);
}

// Command-line-driven, non-interactive mode used by tools/run_ablation.ps1
// (--benchmark: prints one CSV line of steady-state FPS stats, then exits)
// and tools/capture_demo.ps1 (--screenshot-at / --gif: writes a PNG/GIF of a
// scripted scenario). Every field defaults to reproducing today's plain
// interactive run when no flags are passed -- this struct only widens what
// the app can do, it doesn't change default behavior.
struct RunConfig {
    bool hidden = false;
    bool benchmark = false;

    int spawnCount = -1;             // -1 = use the normal timer-driven spawner
    float restitution = 1.0f;
    bool uniformGravity = false;     // false = OrbitSpring (today's default)
    glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);
    float springStrength = 10.0f;
    bool bruteForceCollision = false; // false = BroadPhase (today's default)
    float tunnelSafetyFactor = 0.9f; // see PhysicsWorld::SetTunnelSafetyFactor
    unsigned int seed = 0;
    bool hasSeed = false;

    float duration = -1.0f;          // -1 = run until the window is closed
    float warmup = 0.0f;
    std::vector<float> fpsThresholds = { 60.0f, 30.0f, 15.0f };

    bool hasScreenshot = false;
    float screenshotAt = 0.0f;
    std::string screenshotPath;

    bool hasGif = false;
    float gifStart = 0.0f;
    float gifEnd = 0.0f;
    std::string gifPath;
    int gifFps = 10;

    int trackIndex = -1;

    bool NonInteractive() const { return benchmark || hasScreenshot || hasGif; }
};

namespace {
    std::vector<std::string> Split(const std::string& s, char delim) {
        std::vector<std::string> parts;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) parts.push_back(item);
        return parts;
    }

    // Splits "--key=value" into (key, value); returns false for a bare flag
    // ("--hidden") with no '='.
    bool SplitFlag(const std::string& arg, std::string& key, std::string& value) {
        size_t eq = arg.find('=');
        if (eq == std::string::npos) { key = arg; return false; }
        key = arg.substr(0, eq);
        value = arg.substr(eq + 1);
        return true;
    }
}

RunConfig ParseArgs(int argc, char** argv) {
    RunConfig cfg;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        std::string key, value;
        bool hasValue = SplitFlag(arg, key, value);

        if (key == "--hidden") cfg.hidden = true;
        else if (key == "--benchmark") cfg.benchmark = true;
        else if (key == "--spheres" && hasValue) cfg.spawnCount = std::atoi(value.c_str());
        else if (key == "--restitution" && hasValue) cfg.restitution = std::strtof(value.c_str(), nullptr);
        else if (key == "--gravity-mode" && hasValue) cfg.uniformGravity = (value == "uniform");
        else if (key == "--gravity" && hasValue) {
            std::vector<std::string> parts = Split(value, ',');
            if (parts.size() == 3) {
                cfg.gravity = glm::vec3(std::strtof(parts[0].c_str(), nullptr),
                    std::strtof(parts[1].c_str(), nullptr), std::strtof(parts[2].c_str(), nullptr));
            }
        } else if (key == "--spring" && hasValue) cfg.springStrength = std::strtof(value.c_str(), nullptr);
        else if (key == "--collision" && hasValue) cfg.bruteForceCollision = (value == "bruteforce");
        else if (key == "--tunnel-safety" && hasValue) cfg.tunnelSafetyFactor = std::strtof(value.c_str(), nullptr);
        else if (key == "--seed" && hasValue) { cfg.seed = static_cast<unsigned int>(std::strtoul(value.c_str(), nullptr, 10)); cfg.hasSeed = true; }
        else if (key == "--duration" && hasValue) cfg.duration = std::strtof(value.c_str(), nullptr);
        else if (key == "--warmup" && hasValue) cfg.warmup = std::strtof(value.c_str(), nullptr);
        else if (key == "--fps-thresholds" && hasValue) {
            cfg.fpsThresholds.clear();
            for (const std::string& p : Split(value, ',')) cfg.fpsThresholds.push_back(std::strtof(p.c_str(), nullptr));
        } else if (key == "--screenshot-at" && hasValue) {
            // '@' (not ':') as the separator: a Windows absolute path
            // ("E:\...") already contains a colon after the drive letter,
            // which would make Split(value, ':') see a spurious extra part.
            std::vector<std::string> parts = Split(value, '@');
            if (parts.size() == 2) {
                cfg.hasScreenshot = true;
                cfg.screenshotAt = std::strtof(parts[0].c_str(), nullptr);
                cfg.screenshotPath = parts[1];
            }
        } else if (key == "--gif" && hasValue) {
            std::vector<std::string> parts = Split(value, '@'); // see --screenshot-at's note on why not ':'
            if (parts.size() == 3) {
                cfg.hasGif = true;
                cfg.gifStart = std::strtof(parts[0].c_str(), nullptr);
                cfg.gifEnd = std::strtof(parts[1].c_str(), nullptr);
                cfg.gifPath = parts[2];
            }
        } else if (key == "--gif-fps" && hasValue) cfg.gifFps = std::atoi(value.c_str());
        else if (key == "--track-index" && hasValue) cfg.trackIndex = std::atoi(value.c_str());
    }
    return cfg;
}

int main(int argc, char** argv) {
    RunConfig config = ParseArgs(argc, argv);

    Window window(1600, 1200, "Application", /*visible=*/!config.hidden);
    if (!window.IsValid()) return -1;
    Input input(window.GetHandle());
    DebugUI debugUI(window.GetHandle());

    // Build shader program from GLSL source files (src/shaders/basic.vert,
    // basic.frag -- copied next to the .exe by a post-build step; see
    // Shader::FromFiles for how the path is resolved).
    Shader shader = Shader::FromFiles("shaders/basic.vert", "shaders/basic.frag");
    Camera camera(glm::vec3(0.0f, 0.0f, 100.0f), glm::vec3(0.0, 0.0f, -1.0f), 0);

    // Create meshes for sphere and wall
    Sphere_mesh sphereMesh_high = get_sphere_mesh(1.0f, 25, 15);
    Cuboid_mesh wallMesh(60.0f, 60.0f, 1.0f);

    // Set up random number generators. --seed makes CLI-driven runs
    // (tools/run_ablation.ps1, tools/capture_demo.ps1) reproducible; plain
    // interactive runs stay nondeterministic as before.
    std::random_device rd;
    std::mt19937 gen(config.hasSeed ? config.seed : rd());

    // Scene owns the dynamic spheres, the static walls, and the spawn timer
    // that grows the sphere list over time (see scene/Scene.h).
    Scene scene;
    scene.BuildRoom(&wallMesh);

    // collisionCellSize must stay >= the diameter of the largest sphere spawned
    // below (sphereMesh_high has radius 1.0f -> diameter 2.0f; see UniformGrid's
    // correctness note in physics/collision/BroadPhase.h).
    //
    // numThreads is sized from the actual machine's hardware concurrency rather
    // than a fixed literal, so PhysicsWorld's JobSystem neither oversubscribes
    // (more worker threads than cores) nor leaves available parallelism unused.
    // hardware_concurrency() is allowed by the standard to
    // return 0 if it can't be determined; fall back to a small fixed count
    // rather than constructing a zero-worker (permanently-deadlocked) pool.
    unsigned int hardwareThreads = std::thread::hardware_concurrency();
    int numThreads = (hardwareThreads == 0) ? 4 : static_cast<int>(hardwareThreads);
    // wallThickness read back from wallMesh itself (not a second hardcoded
    // literal) for the same reason Scene::BuildRoom does -- see its comment;
    // feeds PhysicsWorld's tunneling guard (SetTunnelSafetyFactor).
    PhysicsWorld physicsWorld(/*iterations=*/5, numThreads, /*collisionCellSize=*/2.0f, /*wallThickness=*/wallMesh.getHeight());

    // Apply CLI-configured tunables -- all default to today's
    // interactive behavior when no flags are passed.
    physicsWorld.SetRestitution(config.restitution);
    physicsWorld.SetAccelerationMode(config.uniformGravity ? AccelerationMode::UniformGravity : AccelerationMode::OrbitSpring);
    physicsWorld.SetGravity(config.gravity);
    physicsWorld.SetSpringStrength(config.springStrength);
    physicsWorld.SetCollisionMode(config.bruteForceCollision ? CollisionMode::BruteForce : CollisionMode::BroadPhase);
    physicsWorld.SetTunnelSafetyFactor(config.tunnelSafetyFactor);

    Selection selection;
    if (config.spawnCount >= 0) {
        // Place the exact requested count up front (deterministic with
        // --seed) instead of waiting out the timer-driven spawner, and cap
        // the timer spawner at the same count so it never adds more.
        scene.spawnParams.maxSpheres = static_cast<size_t>(config.spawnCount);
        scene.SpawnBatch(&sphereMesh_high, static_cast<size_t>(config.spawnCount), gen);
        if (config.trackIndex >= 0 && static_cast<size_t>(config.trackIndex) < scene.spheres.size()) {
            selection.Set(scene.spheres[config.trackIndex].body.id);
        }
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    float lastFrame = 0.0f;
    float deltaTime = 0.0f;
    VelocityTool velocityTool;
    const glm::vec3 selectionHighlight(1.0f, 1.0f, 0.0f);
    const float dragVelocityScale = 0.05f; // pixels of drag -> world units/sec
    const float dragDeadzonePixels = 3.0f; // below this, treat as a click, not a throw
    float wallAlpha = 0.2f; // semi-transparent by default so the room boundary is visible but not opaque

    // FPS smoothing -- also drives --benchmark's steady-state
    // measurement and threshold-crossing detection below.
    FrameStats frameStats;
    bool nonInteractive = config.NonInteractive();
    float simTime = 0.0f;
    float measureSum = 0.0f;
    float measureMin = std::numeric_limits<float>::max();
    int measureCount = 0;
    struct ThresholdCrossing { float threshold; float crossedAt = -1.0f; };
    std::vector<ThresholdCrossing> thresholdCrossings;
    for (float t : config.fpsThresholds) thresholdCrossings.push_back({ t });

    bool screenshotDone = false;
    GifRecorder gifRecorder;
    bool gifStarted = false;
    bool gifEnded = false;
    float lastGifCaptureTime = 0.0f;

    // Render loop
    while (!window.ShouldClose()) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        simTime += deltaTime;
        frameStats.Update(deltaTime);

        debugUI.BeginFrame();
        // Once ImGui has a widget hovered/focused, our own game-input
        // handling below must not also react to the same click/keypress
        // (e.g. dragging a slider shouldn't also drag-throw the selected
        // sphere). Read right after BeginFrame so this frame's hover state
        // (computed from last frame's layout) is current.
        bool uiWantsMouse = ImGui::GetIO().WantCaptureMouse;
        bool uiWantsKeyboard = ImGui::GetIO().WantCaptureKeyboard;

        // Polled unconditionally (not inside the `!uiWantsKeyboard` check
        // below) so WasKeyJustPressed's up->down edge tracking never skips a
        // frame -- skipping the poll while the UI has focus would leave
        // stale state and could misfire a "just pressed" on the frame focus
        // returns, even for a key that had already been held. Gate the
        // resulting *action*, not the poll itself.
        bool pJustPressed = input.WasKeyJustPressed(GLFW_KEY_P);
        bool rJustPressed = input.WasKeyJustPressed(GLFW_KEY_R);
        bool fJustPressed = input.WasKeyJustPressed(GLFW_KEY_F);
        bool nJustPressed = input.WasKeyJustPressed(GLFW_KEY_N);
        bool deleteJustPressed = input.WasKeyJustPressed(GLFW_KEY_DELETE);

        // P: toggle pause. R: reset. F: toggle fixed on the selected sphere.
        // Delete: remove the selected sphere. N (single-step) is handled
        // near physicsWorld.Step() below, since it only makes sense there.
        if (!uiWantsKeyboard) {
            if (pJustPressed) physicsWorld.SetPaused(!physicsWorld.IsPaused());
            if (rJustPressed) { scene.Reset(); selection.Clear(); }
            if (fJustPressed && selection.HasSelection()) {
                Sphere* selected = scene.FindById(selection.Get());
                if (selected) selected->body.ToggleFixed();
            }
            if (deleteJustPressed && selection.HasSelection()) {
                scene.RemoveById(selection.Get());
                selection.Clear();
            }
        }

        scene.SpawnSpheres(&sphereMesh_high, deltaTime, gen);

        // Clear color and depth buffers.
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader and set view/projection matrices.
        // Hold right mouse button to capture the cursor for mouse-look;
        // released, the cursor is free for clicking/selecting.
        // Only call SetCursorCaptured on an actual state change -- it resets
        // Input's mouse-delta tracking, so calling it every frame regardless
        // would zero out the mouse delta every frame and break mouse-look.
        if (!uiWantsMouse) {
            bool wantCaptured = input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
            if (wantCaptured != input.IsCursorCaptured()) input.SetCursorCaptured(wantCaptured);
        }

        // GetContentWidth/Height (screen coordinates), not GetWidth/Height
        // (framebuffer pixels) -- glfwGetCursorPos (Input::GetMousePosition)
        // reports in screen coordinates, so picking must be sized against
        // the same units or it's systematically off whenever the display's
        // DPI/content scale isn't 100% (a real bug a user hit).
        glm::vec2 screenSize(static_cast<float>(window.GetContentWidth()), static_cast<float>(window.GetContentHeight()));

        // Middle click (cursor free, not over the UI): place a new sphere
        // along the camera ray at a fixed distance, and select it. Not
        // gated by spawnParams.maxSpheres -- that cap limits the automatic
        // timer-driven spawner, not a deliberate one-at-a-time user action.
        if (!uiWantsMouse && !input.IsCursorCaptured() && input.WasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_MIDDLE)) {
            const float placementDistance = 15.0f;
            Ray ray = camera.ScreenPointToRay(input.GetMousePosition(), screenSize);
            glm::vec3 spawnPos = ray.origin + ray.direction * placementDistance;
            selection.Set(scene.SpawnSphereAt(&sphereMesh_high, spawnPos, glm::vec3(0.0f)));
        }

        // Left click (cursor free, not mouse-looking, not over the UI):
        // clicking the already-selected sphere begins a velocity-drag;
        // clicking anything else (or empty space) re-selects
        // (PickClosestSphere returns 0 on a miss, which correctly clears
        // selection too).
        if (!uiWantsMouse && !input.IsCursorCaptured() && input.WasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            glm::vec2 mousePos = input.GetMousePosition();
            Ray ray = camera.ScreenPointToRay(mousePos, screenSize);
            uint32_t hitId = PickClosestSphere(ray, scene.spheres);
            if (hitId != 0 && hitId == selection.Get()) {
                velocityTool.BeginDrag(mousePos);
            } else {
                selection.Set(hitId);
            }
        }

        // Release ends the drag and imbues velocity, unless the drag never
        // moved past the deadzone (then it was just a click, do nothing --
        // otherwise re-clicking the selected sphere would zero its velocity).
        if (velocityTool.IsDragging() && !input.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            glm::vec2 dragDelta = input.GetMousePosition() - velocityTool.GetDragStart();
            if (glm::length(dragDelta) >= dragDeadzonePixels) {
                Sphere* selected = scene.FindById(selection.Get());
                if (selected) {
                    glm::vec3 velocity = (camera.getRight() * dragDelta.x - camera.getUp() * dragDelta.y) * dragVelocityScale;
                    selected->body.SetVelocity(velocity);
                }
            }
            velocityTool.CancelDrag();
        }

        // Skipped entirely in --benchmark/--screenshot-at/--gif runs so
        // captures are clean and dozens of benchmark launches don't pay
        // ImGui layout cost for a panel nothing will ever see.
        if (!nonInteractive) {
            // Explicit default position/size (top-left, with margin) and a
            // minimum size so the panel's content is never clipped -- with
            // imgui.ini persistence off (see DebugUI), this is the position/size
            // it gets every run unless the user manually resizes it that frame.
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(380.0f, 520.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("Physix Debug");
            ImGui::Text("Objects: %zu", scene.spheres.size());
            // Smoothed (EMA), not raw 1/deltaTime -- the raw value visibly
            // flickers frame to frame (see core/FrameStats.h).
            ImGui::Text("FPS: %.0f", frameStats.GetSmoothedFPS());

            bool paused = physicsWorld.IsPaused();
            if (ImGui::Checkbox("Paused (P)", &paused)) physicsWorld.SetPaused(paused);

            float restitution = physicsWorld.GetRestitution();
            if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f)) physicsWorld.SetRestitution(restitution);

            int accelMode = (physicsWorld.GetAccelerationMode() == AccelerationMode::UniformGravity) ? 1 : 0;
            const char* accelModes[] = { "Orbit Spring", "Uniform Gravity" };
            if (ImGui::Combo("Acceleration", &accelMode, accelModes, 2)) {
                physicsWorld.SetAccelerationMode(accelMode == 1 ? AccelerationMode::UniformGravity : AccelerationMode::OrbitSpring);
            }
            if (accelMode == 0) {
                float spring = physicsWorld.GetSpringStrength();
                if (ImGui::SliderFloat("Spring Strength", &spring, 0.0f, 50.0f)) physicsWorld.SetSpringStrength(spring);
            } else {
                glm::vec3 g = physicsWorld.GetGravity();
                float gArr[3] = { g.x, g.y, g.z };
                if (ImGui::SliderFloat3("Gravity", gArr, -30.0f, 30.0f)) physicsWorld.SetGravity(glm::vec3(gArr[0], gArr[1], gArr[2]));
            }

            int collisionMode = (physicsWorld.GetCollisionMode() == CollisionMode::BruteForce) ? 1 : 0;
            const char* collisionModes[] = { "Broad Phase (grid)", "Brute Force (O(n^2))" };
            if (ImGui::Combo("Collision Mode", &collisionMode, collisionModes, 2)) {
                physicsWorld.SetCollisionMode(collisionMode == 1 ? CollisionMode::BruteForce : CollisionMode::BroadPhase);
            }

            // Tunneling guard -- see PhysicsWorld::SetTunnelSafetyFactor's
            // comment. Lower = safer against fast spheres passing through
            // walls, at the cost of an earlier speed clamp.
            float tunnelSafety = physicsWorld.GetTunnelSafetyFactor();
            if (ImGui::SliderFloat("Tunnel Safety Factor", &tunnelSafety, 0.1f, 1.0f)) physicsWorld.SetTunnelSafetyFactor(tunnelSafety);

            float spawnInterval = scene.spawnParams.spawnInterval;
            if (ImGui::SliderFloat("Spawn Interval (s)", &spawnInterval, 0.05f, 5.0f)) scene.spawnParams.spawnInterval = spawnInterval;
            int maxSpheres = static_cast<int>(scene.spawnParams.maxSpheres);
            if (ImGui::SliderInt("Max Spheres", &maxSpheres, 1, 500)) scene.spawnParams.maxSpheres = static_cast<size_t>(maxSpheres);

            ImGui::SliderFloat("Wall Alpha", &wallAlpha, 0.0f, 1.0f);

            // Per-object editing: every parameter here writes straight to the
            // selected Sphere's RigidBody/RenderComponent fields -- no new
            // physics/render code needed, same as freeze and drag-to-throw.
            if (selection.HasSelection()) {
                ImGui::Separator();
                Sphere* selected = scene.FindById(selection.Get());
                if (selected) {
                    ImGui::Text("Selected #%u", selection.Get());

                    bool fixed = selected->body.fixed;
                    if (ImGui::Checkbox("Fixed (F)", &fixed)) selected->body.ToggleFixed();

                    float mass = selected->body.mass;
                    if (ImGui::SliderFloat("Mass", &mass, 0.1f, 20.0f)) selected->body.mass = mass;

                    float color[3] = { selected->render.color.r, selected->render.color.g, selected->render.color.b };
                    if (ImGui::ColorEdit3("Color", color)) selected->render.SetColor(glm::vec3(color[0], color[1], color[2]));

                    float transparency = selected->render.transparency;
                    if (ImGui::SliderFloat("Transparency", &transparency, 0.0f, 1.0f)) selected->render.SetTransparency(transparency);

                    glm::vec3 vel = selected->body.velocity;
                    float velArr[3] = { vel.x, vel.y, vel.z };
                    if (ImGui::DragFloat3("Velocity", velArr, 0.1f)) selected->body.SetVelocity(glm::vec3(velArr[0], velArr[1], velArr[2]));

                    // Last: RemoveById invalidates `selected` (swap-and-pop), so
                    // nothing below this may touch it.
                    if (ImGui::Button("Delete (Del)")) {
                        scene.RemoveById(selection.Get());
                        selection.Clear();
                    }
                }
            }
            ImGui::End();
        }

        shader.use(); camera.Update(input, deltaTime);
        float aspect = window.GetAspectRatio();

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix(aspect);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // Set lighting uniforms.
        shader.setVec3("lightPos", camera.getPosition());
        shader.setVec3("viewPos", glm::vec3(0.0f, 0.0f, 3.0f));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Now render spheres (after transparent objects if blending is enabled)
        std::sort(scene.spheres.begin(), scene.spheres.end(), [&camera](const Sphere& a, const Sphere& b) {
            return camera.distanceFromCameraPlane(a.body.position) > camera.distanceFromCameraPlane(b.body.position);
            });
        for (Sphere& s : scene.spheres) {
            bool isSelected = s.body.id != 0 && s.body.id == selection.Get();
            shader.setVec3("objectColor", isSelected ? selectionHighlight : s.render.color);
            shader.setFloat("alpha", s.render.transparency);
            s.Render(shader);
        }

        for (Cuboid& wall : scene.walls) {
            shader.setVec3("objectColor", glm::vec3(0.5f));
            shader.setFloat("alpha", wallAlpha);
            wall.Render(shader);
        }

        // --benchmark steady-state measurement: accumulate the smoothed FPS
        // once past the warmup window, and record the sim time each
        // requested threshold is first crossed (see tools/run_ablation.ps1).
        if (config.benchmark && simTime >= config.warmup) {
            float fps = frameStats.GetSmoothedFPS();
            measureSum += fps;
            measureMin = std::min(measureMin, fps);
            measureCount++;
            for (ThresholdCrossing& tc : thresholdCrossings) {
                if (tc.crossedAt < 0.0f && fps < tc.threshold) tc.crossedAt = simTime;
            }
        }

        // --screenshot-at / --gif: read the just-rendered framebuffer (must
        // happen before SwapBuffers, while this frame's image is still the
        // one bound). See core/Capture.h.
        if (config.hasScreenshot && !screenshotDone && simTime >= config.screenshotAt) {
            Capture::SaveScreenshotPNG(config.screenshotPath, static_cast<int>(window.GetWidth()), static_cast<int>(window.GetHeight()));
            screenshotDone = true;
        }
        if (config.hasGif && !gifEnded) {
            if (!gifStarted && simTime >= config.gifStart) {
                gifStarted = gifRecorder.Begin(config.gifPath, static_cast<int>(window.GetWidth()), static_cast<int>(window.GetHeight()), config.gifFps);
                lastGifCaptureTime = simTime;
            }
            if (gifStarted) {
                if (simTime > config.gifEnd) {
                    gifRecorder.End();
                    gifEnded = true;
                } else if (simTime - lastGifCaptureTime >= 1.0f / static_cast<float>(config.gifFps)) {
                    gifRecorder.CaptureFrame();
                    lastGifCaptureTime = simTime;
                }
            }
        }

        physicsWorld.Step(scene.spheres, scene.walls, deltaTime);
        if (physicsWorld.IsPaused() && !uiWantsKeyboard && nJustPressed) {
            physicsWorld.StepOnce(scene.spheres, scene.walls, deltaTime);
        }

        debugUI.EndFrame();

        // Swap buffers and poll IO events.
        window.SwapBuffers();
        window.PollEvents();

        if (config.duration >= 0.0f && simTime >= config.duration) break;
    }
    gifRecorder.End(); // no-op if not recording (duration elapsed mid-gif, or --gif not set)

    if (config.benchmark) {
        // One CSV line for tools/run_ablation.ps1 to parse: spheres,
        // restitution, gravity_mode, collision_mode, avg_fps, min_fps, then
        // one first-crossing-time column per --fps-thresholds entry (-1 if
        // never crossed within --duration).
        float avgFps = measureCount > 0 ? measureSum / static_cast<float>(measureCount) : 0.0f;
        float minFps = measureCount > 0 ? measureMin : 0.0f;
        std::printf("%d,%.3f,%s,%s,%.3f,%.3f", static_cast<int>(scene.spheres.size()), config.restitution,
            config.uniformGravity ? "uniform" : "orbit", config.bruteForceCollision ? "bruteforce" : "broadphase",
            avgFps, minFps);
        for (const ThresholdCrossing& tc : thresholdCrossings) {
            std::printf(",%.3f", tc.crossedAt);
        }
        std::printf("\n");
    }

    // Window's destructor handles glfwDestroyWindow/glfwTerminate (RAII).
    return 0;
}
