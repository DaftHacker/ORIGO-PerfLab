#pragma once

#include "ScriptSystem.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

// ORIGO's project registry generator uses this annotation to discover the game mode.
ORIGO_GAME_MODE_CLASS(PerfLabGameMode)
class PerfLabGameMode final : public Origo::GameMode {
public:
    void OnEnter(Origo::Scene& scene, Origo::GameSession&) override {
        m_elapsedSeconds = 0.0f;
        m_particleAccumulator = 0.0f;
        m_smoothedTickMs = 0.0f;
        m_haveTickSample = false;
        m_lastBuildMs = 0.0f;
        m_cameraWasSweeping = false;
        m_sweepAngle = 0.0f;

        CaptureHomeCamera(scene);
        m_cameraSweep = ReadBoolEnv("ORIGO_PERFLAB_CAMERA_SWEEP", false);
        m_hudEnabled = ReadBoolEnv("ORIGO_PERFLAB_HUD", true);

        m_mode = ParseScenario(std::getenv("ORIGO_PERFLAB_SCENARIO"));
        int defaultCount = 1000;
        if (m_mode == Scenario::Physics) defaultCount = 100;
        else if (m_mode == Scenario::Particles) defaultCount = 5000;
        else if (m_mode == Scenario::Empty) defaultCount = 0;
        m_targetCount = ReadIntEnv("ORIGO_PERFLAB_COUNT", defaultCount, 0, 50000);
        if (m_mode == Scenario::Physics) m_targetCount = std::clamp(m_targetCount, 1, 1000);
        if (m_mode == Scenario::Particles) m_targetCount = std::clamp(m_targetCount, 100, 50000);
        if (m_mode == Scenario::Static || m_mode == Scenario::Moving)
            m_targetCount = std::clamp(m_targetCount, 1, 50000);
        RebuildPopulation(scene, m_mode, m_targetCount);
    }

    void OnUpdate(Origo::Scene& scene, Origo::GameSession&, Origo::InputSystem&, float deltaTime) override {
        const float dt = std::clamp(deltaTime, 0.0f, 0.25f);
        m_elapsedSeconds += dt;
        const float tickSampleMs = std::max(0.0f, deltaTime) * 1000.0f;
        if (!m_haveTickSample) {
            m_smoothedTickMs = tickSampleMs;
            m_haveTickSample = true;
        } else {
            m_smoothedTickMs += (tickSampleMs - m_smoothedTickMs) * 0.06f;
        }

        if (m_mode == Scenario::Moving) UpdateMovingObjects(scene, dt);
        if (m_mode == Scenario::Particles) UpdateContinuousParticles(scene, dt);
        UpdateCamera(scene, dt);

        if (m_hudEnabled) DrawHud(scene);
    }

    void OnExit(Origo::Scene& scene, Origo::GameSession&) override {
        ClearPopulation(scene);
    }

private:
    enum class Scenario { Static, Moving, Physics, Particles, Empty };

    struct MovingItem {
        std::uint32_t id = 0;
        float baseZ = 0.0f;
        float baseRotation = 0.0f;
        float phase = 0.0f;
    };

    static bool IsPerfLabObject(const Origo::GameObject& object) {
        return object.name.compare(0, 8, "PerfLab:") == 0;
    }

    // Do not call Scene::CreateObject or Scene::DestroyObject across the game-module
    // DLL boundary. Append/remove the public scene records in batches and invalidate
    // the Scene's inline object-index cache, following the engine's cache contract.
    static Origo::GameObject& AppendSceneObject(Origo::Scene& scene, std::string name) {
        Origo::GameObject object{};
        object.id = scene.nextId++;
        object.parentId = 0;
        object.name = std::move(name);
        object.active = true;
        scene.objects.push_back(std::move(object));
        return scene.objects.back();
    }

    static Origo::Vec3 Normalize(Origo::Vec3 value) {
        const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
        if (!std::isfinite(length) || length < 1.0e-5f) return {1.0f, 0.0f, 0.0f};
        return {value.x / length, value.y / length, value.z / length};
    }

    static int ReadIntEnv(const char* name, int fallback, int minimum, int maximum) {
        const char* value = std::getenv(name);
        if (!value || !*value) return fallback;
        char* end = nullptr;
        const long parsed = std::strtol(value, &end, 10);
        if (end == value || !end || *end != '\0') return fallback;
        return static_cast<int>(std::clamp(parsed, static_cast<long>(minimum), static_cast<long>(maximum)));
    }

    static bool ReadBoolEnv(const char* name, bool fallback) {
        const char* value = std::getenv(name);
        if (!value || !*value) return fallback;
        if (std::strcmp(value, "0") == 0 || std::strcmp(value, "false") == 0 ||
            std::strcmp(value, "False") == 0 || std::strcmp(value, "off") == 0 ||
            std::strcmp(value, "OFF") == 0) return false;
        return true;
    }

    static Scenario ParseScenario(const char* value) {
        if (!value || !*value) return Scenario::Static;
        if (std::strcmp(value, "static") == 0) return Scenario::Static;
        if (std::strcmp(value, "moving") == 0) return Scenario::Moving;
        if (std::strcmp(value, "physics") == 0) return Scenario::Physics;
        if (std::strcmp(value, "particles") == 0) return Scenario::Particles;
        if (std::strcmp(value, "empty") == 0) return Scenario::Empty;
        return Scenario::Static;
    }

    static const char* ScenarioName(Scenario scenario) {
        switch (scenario) {
        case Scenario::Static: return "Static grid";
        case Scenario::Moving: return "Moving grid";
        case Scenario::Physics: return "Physics stack";
        case Scenario::Particles: return "Particles";
        case Scenario::Empty: return "Empty";
        }
        return "Unknown";
    }

    void CaptureHomeCamera(Origo::Scene& scene) {
        const Origo::GameObject* camera = scene.FindObjectByName("PerfLab Camera");
        m_cameraId = camera ? camera->id : 0;
        if (!camera) {
            m_cameraHomePosition = {-55.0f, -70.0f, 37.0f};
        } else {
            m_cameraHomePosition = {camera->transform.x, camera->transform.y, camera->transform.z};
        }
        m_cameraHomeForward = Normalize({-m_cameraHomePosition.x, -m_cameraHomePosition.y, -m_cameraHomePosition.z});
    }

    void ClearPopulation(Origo::Scene& scene) {
        scene.objects.erase(
            std::remove_if(scene.objects.begin(), scene.objects.end(),
                [](const Origo::GameObject& object) { return IsPerfLabObject(object); }),
            scene.objects.end());
        scene.objectIndexCacheSize = static_cast<std::size_t>(-1);
        scene.dirty = true;
        m_movingItems.clear();
        m_targetCount = 0;
        m_liveObjectCount = 0;
        m_particleAccumulator = 0.0f;
    }

    void RebuildPopulation(Origo::Scene& scene, Scenario scenario, int count) {
        const auto start = std::chrono::steady_clock::now();
        ClearPopulation(scene);
        m_mode = scenario;
        m_targetCount = scenario == Scenario::Empty ? 0 : count;
        m_liveObjectCount = 0;

        if (scenario == Scenario::Physics) AppendFloor(scene);
        if (scenario == Scenario::Static || scenario == Scenario::Moving || scenario == Scenario::Physics) {
            const int safeCount = scenario == Scenario::Physics
                ? std::clamp(count, 1, 1000)
                : std::clamp(count, 1, 50000);
            const int physicsLayers = scenario == Scenario::Physics ? 4 : 1;
            const int perLayer = (safeCount + physicsLayers - 1) / physicsLayers;
            const int columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(perLayer)))));
            const int rows = (perLayer + columns - 1) / columns;
            const float spacing = scenario == Scenario::Physics ? 1.25f : 1.8f;
            const float scale = scenario == Scenario::Physics ? 0.78f : 0.7f;

            if (scenario == Scenario::Moving) m_movingItems.reserve(static_cast<std::size_t>(safeCount));
            for (int index = 0; index < safeCount; ++index) {
                const int layer = index / perLayer;
                const int cell = index % perLayer;
                const int column = cell % columns;
                const int row = cell / columns;
                const float x = (static_cast<float>(column) - static_cast<float>(columns - 1) * 0.5f) * spacing;
                const float y = (static_cast<float>(row) - static_cast<float>(rows - 1) * 0.5f) * spacing;
                const float z = scenario == Scenario::Physics
                    ? 1.2f + static_cast<float>(layer) * 1.15f
                    : 0.45f;

                Origo::GameObject& object = AppendSceneObject(scene, "PerfLab:Cube:" + std::to_string(index));
                object.mobility = scenario == Scenario::Static ? Origo::ObjectMobility::Static : Origo::ObjectMobility::Movable;
                object.transform.x = x;
                object.transform.y = y;
                object.transform.z = z;
                object.transform.sx = scale;
                object.transform.sy = scale;
                object.transform.sz = scale;

                // Zero mesh/material IDs intentionally select the engine-owned default
                // cube and material. The repeated surface state exercises ORIGO's normal
                // render batching without depending on external assets.
                object.hasMeshRenderer = true;
                object.meshRenderer.mesh = 0;
                object.meshRenderer.material = 0;
                object.meshRenderer.meshGuid.clear();
                object.meshRenderer.materialGuid.clear();
                object.meshRenderer.castShadows = false;
                object.meshRenderer.receiveShadows = false;

                if (scenario == Scenario::Physics) {
                    object.hasCollider = true;
                    object.collider.shape = Origo::ColliderShape::Box;
                    object.collider.sizeX = 1.0f;
                    object.collider.sizeY = 1.0f;
                    object.collider.sizeZ = 1.0f;
                    object.collider.isTrigger = false;

                    object.hasRigidbody = true;
                    object.rigidbody.enabled = true;
                    object.rigidbody.isKinematic = false;
                    object.rigidbody.useGravity = true;
                    object.rigidbody.mass = 1.0f;
                    object.rigidbody.linearDamping = 0.03f;
                    object.rigidbody.angularDamping = 0.03f;
                }

                if (scenario == Scenario::Moving) {
                    m_movingItems.push_back({object.id, z, 0.0f, static_cast<float>(index) * 0.173f});
                }
                ++m_liveObjectCount;
            }
        }

        scene.objectIndexCacheSize = static_cast<std::size_t>(-1);
        scene.dirty = true;
        m_lastBuildMs = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - start).count();
    }

    void AppendFloor(Origo::Scene& scene) {
        Origo::GameObject& floor = AppendSceneObject(scene, "PerfLab:Floor");
        floor.mobility = Origo::ObjectMobility::Static;
        floor.transform.x = 0.0f;
        floor.transform.y = 0.0f;
        floor.transform.z = -0.55f;
        floor.transform.sx = 120.0f;
        floor.transform.sy = 120.0f;
        floor.transform.sz = 0.5f;
        floor.hasMeshRenderer = true;
        floor.meshRenderer.mesh = 0;
        floor.meshRenderer.material = 0;
        floor.meshRenderer.castShadows = false;
        floor.meshRenderer.receiveShadows = false;
        floor.hasCollider = true;
        floor.collider.shape = Origo::ColliderShape::Box;
        floor.collider.sizeX = floor.collider.sizeY = floor.collider.sizeZ = 1.0f;
        floor.collider.friction = 0.8f;
    }

    void UpdateMovingObjects(Origo::Scene& scene, float dt) {
        if (m_movingItems.empty()) return;
        const float time = m_elapsedSeconds;
        for (const MovingItem& item : m_movingItems) {
            Origo::GameObject* object = scene.FindObject(item.id);
            if (!object) continue;
            object->transform.z = item.baseZ + std::sin(time * 1.3f + item.phase) * 0.65f;
            object->transform.rz = std::fmod(item.baseRotation + time * 48.0f + item.phase * 57.29578f, 360.0f);
        }
        (void)dt;
    }

    void SpawnParticleBurst(Origo::Scene& scene, std::uint32_t count) {
        Origo::RuntimeParticleBurst burst{};
        burst.position = {0.0f, 0.0f, 8.0f};
        burst.gravity = {0.0f, 0.0f, -1.5f};
        burst.count = count;
        burst.lifetimeMin = 2.0f;
        burst.lifetimeMax = 4.0f;
        burst.speedMin = 0.8f;
        burst.speedMax = 5.0f;
        burst.sizeStart = 0.14f;
        burst.sizeEnd = 0.015f;
        burst.startColor = {0.25f, 0.8f, 1.0f, 1.0f};
        burst.endColor = {0.15f, 0.35f, 1.0f, 0.0f};
        scene.SpawnTransientParticleBurst(burst);
    }

    void UpdateContinuousParticles(Origo::Scene& scene, float dt) {
        m_particleAccumulator += dt;
        if (m_particleAccumulator < 0.2f) return;
        m_particleAccumulator = std::fmod(m_particleAccumulator, 0.2f);
        const int burstCount = std::max(1, m_targetCount / 15);
        SpawnParticleBurst(scene, static_cast<std::uint32_t>(burstCount));
    }

    void UpdateCamera(Origo::Scene& scene, float dt) {
        if (!m_cameraId) return;
        if (m_cameraSweep) {
            m_sweepAngle += dt * 0.18f;
            const Origo::Vec3 position{
                std::cos(m_sweepAngle) * 95.0f,
                std::sin(m_sweepAngle) * 95.0f,
                38.0f
            };
            const Origo::Vec3 forward = Normalize({-position.x, -position.y, -position.z});
            scene.SetRuntimeCameraView(m_cameraId, position, forward, Origo::WorldAxes::Up);
            m_cameraWasSweeping = true;
        } else if (m_cameraWasSweeping) {
            scene.SetRuntimeCameraView(m_cameraId, m_cameraHomePosition, m_cameraHomeForward, Origo::WorldAxes::Up);
            m_cameraWasSweeping = false;
        }
    }

    void DrawHud(Origo::Scene& scene) {
        scene.UIPanel("PerfLab.Panel",
            {0.015f, 0.015f, 0.385f, 0.505f},
            {0.025f, 0.035f, 0.055f, 0.88f});

        char status[256]{};
        char timing[256]{};
        std::snprintf(status, sizeof(status), "%s  |  OBJECTS %d",
            ScenarioName(m_mode), m_liveObjectCount);
        std::snprintf(timing, sizeof(timing), "Tick delta %.2f ms  |  Build/clear %.2f ms  |  Camera %s",
            m_smoothedTickMs, m_lastBuildMs, m_cameraSweep ? "sweep" : "fixed");

        scene.UIText("PerfLab.Title", "ORIGO PERFLAB", 0.03f, 0.04f, 0.8f,
            {0.45f, 0.85f, 1.0f, 1.0f});
        scene.UIText("PerfLab.Status", status, 0.03f, 0.075f, 0.48f);
        scene.UIText("PerfLab.Timing", timing, 0.03f, 0.105f, 0.4f);
        scene.UIText("PerfLab.Hint", "For CPU/GPU frame data, use the profiler capture script.", 0.03f, 0.132f, 0.34f,
            {0.78f, 0.82f, 0.9f, 1.0f});

        if (scene.UIButton("PerfLab.Static1K", "STATIC 1K", {0.03f, 0.16f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Static, 1000);
        if (scene.UIButton("PerfLab.Static5K", "STATIC 5K", {0.205f, 0.16f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Static, 5000);
        if (scene.UIButton("PerfLab.Static10K", "STATIC 10K", {0.03f, 0.212f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Static, 10000);
        if (scene.UIButton("PerfLab.Static25K", "STATIC 25K", {0.205f, 0.212f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Static, 25000);
        if (scene.UIButton("PerfLab.Moving1K", "MOVING 1K", {0.03f, 0.264f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Moving, 1000);
        if (scene.UIButton("PerfLab.Moving5K", "MOVING 5K", {0.205f, 0.264f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Moving, 5000);
        if (scene.UIButton("PerfLab.Physics100", "PHYSICS 100", {0.03f, 0.316f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Physics, 100);
        if (scene.UIButton("PerfLab.Physics500", "PHYSICS 500", {0.205f, 0.316f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Physics, 500);
        if (scene.UIButton("PerfLab.Particles5K", "PARTICLES 5K", {0.03f, 0.368f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Particles, 5000);
        if (scene.UIButton("PerfLab.Burst", "PARTICLE BURST", {0.205f, 0.368f, 0.16f, 0.044f}))
            SpawnParticleBurst(scene, 5000);
        if (scene.UIButton("PerfLab.CameraSweep", m_cameraSweep ? "CAMERA FIXED" : "CAMERA SWEEP",
            {0.03f, 0.42f, 0.16f, 0.044f}))
            m_cameraSweep = !m_cameraSweep;
        if (scene.UIButton("PerfLab.Clear", "CLEAR SCENE", {0.205f, 0.42f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Empty, 0);
    }

    Scenario m_mode = Scenario::Static;
    std::vector<MovingItem> m_movingItems;
    std::uint32_t m_cameraId = 0;
    Origo::Vec3 m_cameraHomePosition{-55.0f, -70.0f, 37.0f};
    Origo::Vec3 m_cameraHomeForward{0.5f, 0.6f, -0.5f};
    float m_elapsedSeconds = 0.0f;
    float m_particleAccumulator = 0.0f;
    float m_smoothedTickMs = 0.0f;
    float m_lastBuildMs = 0.0f;
    float m_sweepAngle = 0.0f;
    int m_targetCount = 1000;
    int m_liveObjectCount = 0;
    bool m_haveTickSample = false;
    bool m_hudEnabled = true;
    bool m_cameraSweep = false;
    bool m_cameraWasSweeping = false;
};
