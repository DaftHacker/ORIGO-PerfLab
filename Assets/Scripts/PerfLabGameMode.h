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

// PerfLab's tests use regular ORIGO scene objects intentionally. That exposes the
// true scene traversal / render submission path instead of timing a separate GL demo.
ORIGO_GAME_MODE_CLASS(PerfLabGameMode)
class PerfLabGameMode final : public Origo::GameMode {
public:
    void OnEnter(Origo::Scene& scene, Origo::GameSession&) override {
        m_elapsedSeconds = 0.0f;
        m_particleAccumulator = 0.0f;
        m_churnAccumulator = 0.0f;
        m_smoothedTickMs = 0.0f;
        m_haveTickSample = false;
        m_lastBuildMs = 0.0f;
        m_cameraWasSweeping = false;
        m_sweepAngle = 0.0f;

        ReadAuthoredSettings(scene);
        CaptureResources(scene);
        m_cameraSweep = ReadBoolEnv("ORIGO_PERFLAB_CAMERA_SWEEP", false);
        m_hudEnabled = ReadBoolEnv("ORIGO_PERFLAB_HUD", true);
        m_autoFit = ReadBoolEnv("ORIGO_PERFLAB_AUTO_FIT", true);
        m_mode = ParseScenario(std::getenv("ORIGO_PERFLAB_SCENARIO"));

        int configuredCount = ConfiguredCount(m_mode);
        m_targetCount = ReadIntEnv("ORIGO_PERFLAB_COUNT", configuredCount, 0, 50000);
        ClampCount();
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

        if (m_mode == Scenario::Moving) UpdateMovingObjects(scene);
        if (m_mode == Scenario::Particles) UpdateContinuousParticles(scene, dt);
        if (m_mode == Scenario::Churn) {
            m_churnAccumulator += dt;
            if (m_churnAccumulator >= m_churnInterval) {
                m_churnAccumulator = 0.0f;
                RebuildPopulation(scene, Scenario::Churn, m_targetCount);
            }
        }
        UpdateCamera(scene, dt);
        if (m_hudEnabled) DrawHud(scene);
    }

    void OnExit(Origo::Scene& scene, Origo::GameSession&) override {
        ClearPopulation(scene);
    }

private:
    enum class Scenario { Static, Instances, Moving, Physics, Particles, Lights, Churn, Empty };

    struct MovingItem {
        std::uint32_t id = 0;
        float baseZ = 0.0f;
        float baseRotation = 0.0f;
        float phase = 0.0f;
    };

    static constexpr const char* kSphereGuid = "f3120000000000000000000000000001";
    static constexpr float kBenchmarkDrawDistance = 100000.0f;

    static bool IsPerfLabObject(const Origo::GameObject& object) {
        return object.name.compare(0, 8, "PerfLab:") == 0;
    }

    // Scene::CreateObject is deliberately not used across the game-module DLL
    // boundary. These objects are appended in a batch and the scene index cache is
    // invalidated once at the end of the rebuild.
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
        if (std::strcmp(value, "instances") == 0) return Scenario::Instances;
        if (std::strcmp(value, "moving") == 0) return Scenario::Moving;
        if (std::strcmp(value, "physics") == 0) return Scenario::Physics;
        if (std::strcmp(value, "particles") == 0) return Scenario::Particles;
        if (std::strcmp(value, "lights") == 0) return Scenario::Lights;
        if (std::strcmp(value, "churn") == 0) return Scenario::Churn;
        if (std::strcmp(value, "empty") == 0) return Scenario::Empty;
        return Scenario::Static;
    }

    static const char* ScenarioName(Scenario scenario) {
        switch (scenario) {
        case Scenario::Static: return "GameObject grid";
        case Scenario::Instances: return "Instanced Static Mesh";
        case Scenario::Moving: return "Moving grid";
        case Scenario::Physics: return "Randomized physics";
        case Scenario::Particles: return "Particles";
        case Scenario::Lights: return "Point-light grid";
        case Scenario::Churn: return "Spawn/destroy churn";
        case Scenario::Empty: return "Empty";
        }
        return "Unknown";
    }

    static int ReadIntProperty(const Origo::Scene& scene, const char* name, int fallback) {
        const auto* settings = scene.FindObjectByName("PerfLab Settings");
        if (!settings) return fallback;
        for (const auto& script : settings->scripts) {
            if (script.className != "PerfLabSettings") continue;
            for (const auto& property : script.properties) {
                if (property.name == name && property.value.type == Origo::PropertyType::Int)
                    return property.value.intValue;
            }
        }
        return fallback;
    }

    static float ReadFloatProperty(const Origo::Scene& scene, const char* name, float fallback) {
        const auto* settings = scene.FindObjectByName("PerfLab Settings");
        if (!settings) return fallback;
        for (const auto& script : settings->scripts) {
            if (script.className != "PerfLabSettings") continue;
            for (const auto& property : script.properties) {
                if (property.name == name && property.value.type == Origo::PropertyType::Float)
                    return property.value.floatValue;
            }
        }
        return fallback;
    }

    static bool ReadBoolProperty(const Origo::Scene& scene, const char* name, bool fallback) {
        const auto* settings = scene.FindObjectByName("PerfLab Settings");
        if (!settings) return fallback;
        for (const auto& script : settings->scripts) {
            if (script.className != "PerfLabSettings") continue;
            for (const auto& property : script.properties) {
                if (property.name == name && property.value.type == Origo::PropertyType::Bool)
                    return property.value.boolValue;
            }
        }
        return fallback;
    }

    void ReadAuthoredSettings(const Origo::Scene& scene) {
        m_gameObjectCount = std::clamp(ReadIntProperty(scene, "GameObjectCount", 1000), 100, 50000);
        m_instanceCount = std::clamp(ReadIntProperty(scene, "InstanceCount", 10000), 100, 50000);
        m_movingCount = std::clamp(ReadIntProperty(scene, "MovingCount", 1000), 100, 25000);
        m_physicsCount = std::clamp(ReadIntProperty(scene, "PhysicsCount", 180), 1, 1500);
        m_physicsShapeMode = std::clamp(ReadIntProperty(scene, "PhysicsShapeMode", 2), 0, 2);
        m_rngSeed = static_cast<std::uint32_t>(std::max(1, ReadIntProperty(scene, "PhysicsSeed", 1337)));
        m_physicsRestitution = std::clamp(ReadFloatProperty(scene, "PhysicsRestitution", 0.82f), 0.0f, 1.0f);
        m_physicsImpulse = std::clamp(ReadFloatProperty(scene, "PhysicsImpulse", 4.0f), 0.0f, 30.0f);
        m_particleTarget = std::clamp(ReadIntProperty(scene, "ParticleTarget", 5000), 100, 50000);
        m_lightCount = std::clamp(ReadIntProperty(scene, "LightCount", 16), 0, 128);
        m_lightShadows = ReadBoolProperty(scene, "LightShadows", false);
        m_churnCount = std::clamp(ReadIntProperty(scene, "ChurnCount", 500), 10, 5000);
        m_cameraMargin = std::clamp(ReadFloatProperty(scene, "CameraMargin", 1.2f), 1.0f, 2.5f);
        m_churnInterval = std::clamp(ReadFloatProperty(scene, "ChurnIntervalSeconds", 0.5f), 0.1f, 5.0f);
        m_particleInterval = std::clamp(ReadFloatProperty(scene, "ParticleBurstInterval", 0.2f), 0.05f, 2.0f);
        m_physicsCastShadows = ReadBoolProperty(scene, "PhysicsCastShadows", false);
    }

    void CaptureResources(Origo::Scene& scene) {
        if (const auto* camera = scene.FindObjectByName("PerfLab Camera")) {
            m_cameraId = camera->id;
            m_cameraFov = camera->camera.fieldOfView;
            m_cameraHomePosition = {camera->transform.x, camera->transform.y, camera->transform.z};
            m_cameraHomeForward = Normalize({-m_cameraHomePosition.x, -m_cameraHomePosition.y, -m_cameraHomePosition.z});
        } else {
            m_cameraId = 0;
        }
        if (const auto* sphere = scene.FindObjectByName("PerfLab Sphere Prototype"))
            m_sphereMesh = sphere->meshRenderer.mesh;
        m_rngState = m_rngSeed ? m_rngSeed : 1u;
    }

    int ConfiguredCount(Scenario scenario) const {
        switch (scenario) {
        case Scenario::Static: return m_gameObjectCount;
        case Scenario::Instances: return m_instanceCount;
        case Scenario::Moving: return m_movingCount;
        case Scenario::Physics: return m_physicsCount;
        case Scenario::Particles: return m_particleTarget;
        case Scenario::Lights: return m_lightCount;
        case Scenario::Churn: return m_churnCount;
        case Scenario::Empty: return 0;
        }
        return m_gameObjectCount;
    }

    void ClampCount() {
        switch (m_mode) {
        case Scenario::Static: m_targetCount = std::clamp(m_targetCount, 100, 50000); break;
        case Scenario::Instances: m_targetCount = std::clamp(m_targetCount, 100, 50000); break;
        case Scenario::Moving: m_targetCount = std::clamp(m_targetCount, 100, 25000); break;
        case Scenario::Physics: m_targetCount = std::clamp(m_targetCount, 1, 1500); break;
        case Scenario::Particles: m_targetCount = std::clamp(m_targetCount, 100, 50000); break;
        case Scenario::Lights: m_targetCount = std::clamp(m_targetCount, 0, 128); break;
        case Scenario::Churn: m_targetCount = std::clamp(m_targetCount, 10, 5000); break;
        case Scenario::Empty: m_targetCount = 0; break;
        }
    }

    std::uint32_t RandomU32() {
        std::uint32_t x = m_rngState;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        m_rngState = x ? x : 1u;
        return m_rngState;
    }

    float Random01() {
        return static_cast<float>(RandomU32() & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

    float RandomRange(float low, float high) {
        return low + (high - low) * Random01();
    }

    bool RandomBool(float chance = 0.5f) {
        return Random01() < chance;
    }

    void ClearPopulation(Origo::Scene& scene) {
        std::vector<std::uint32_t> removeIds;
        removeIds.reserve(static_cast<std::size_t>(m_liveObjectCount) + 8);
        for (const auto& object : scene.objects)
            if (IsPerfLabObject(object)) removeIds.push_back(object.id);

        if (scene.runtimeApi.destroyObject) {
            // Use ORIGO's runtime bridge so physics bodies, script state, and renderer
            // caches are invalidated together. Destruction is deferred until this
            // game-mode callback completes, per the Scene API contract.
            for (const std::uint32_t id : removeIds) scene.DestroyRuntimeObject(id);
        } else {
            scene.objects.erase(
                std::remove_if(scene.objects.begin(), scene.objects.end(),
                    [](const Origo::GameObject& object) { return IsPerfLabObject(object); }),
                scene.objects.end());
        }
        scene.objectIndexCacheSize = static_cast<std::size_t>(-1);
        scene.dirty = true;
        m_movingItems.clear();
        m_liveObjectCount = 0;
        m_particleAccumulator = 0.0f;
    }

    void ConfigureRenderable(Origo::GameObject& object, bool castShadows=false, bool receiveShadows=false) {
        object.hasMeshRenderer = true;
        object.meshRenderer.mesh = 0;
        object.meshRenderer.material = 0;
        object.meshRenderer.meshGuid.clear();
        object.meshRenderer.materialGuid.clear();
        object.meshRenderer.castShadows = castShadows;
        object.meshRenderer.receiveShadows = receiveShadows;
        // Keep benchmark objects in range even when the camera fits a very large
        // population. The old size-based auto-distance caused edge cubes to vanish.
        object.meshRenderer.runtimeMaxDrawDistance = kBenchmarkDrawDistance;
    }

    void RebuildPopulation(Origo::Scene& scene, Scenario scenario, int count) {
        const auto start = std::chrono::steady_clock::now();
        ClearPopulation(scene);
        m_mode = scenario;
        m_targetCount = scenario == Scenario::Empty ? 0 : count;
        ClampCount();
        m_rngState = m_rngSeed ? m_rngSeed : 1u;
        m_rngState ^= static_cast<std::uint32_t>(m_targetCount * 747796405u + static_cast<unsigned>(m_mode) * 2891336453u);
        if (!m_rngState) m_rngState = 1u;

        if (scenario == Scenario::Physics) {
            AppendPhysicsFloor(scene, m_targetCount);
            AppendPhysicsPopulation(scene, m_targetCount);
        } else if (scenario == Scenario::Static || scenario == Scenario::Moving || scenario == Scenario::Churn) {
            AppendObjectPopulation(scene, scenario, m_targetCount);
        } else if (scenario == Scenario::Instances) {
            AppendInstancePopulation(scene, m_targetCount);
        } else if (scenario == Scenario::Particles) {
            // The effects are spawned in OnUpdate; no scene objects are needed.
            m_liveObjectCount = 0;
        } else if (scenario == Scenario::Lights) {
            AppendLightingPopulation(scene, m_targetCount);
        }

        scene.objectIndexCacheSize = static_cast<std::size_t>(-1);
        scene.dirty = true;
        m_lastBuildMs = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        if (m_autoFit) FitCamera(scene);
    }

    void AppendObjectPopulation(Origo::Scene& scene, Scenario scenario, int count) {
        const int safeCount = scenario == Scenario::Churn
            ? std::clamp(count, 10, 5000)
            : std::clamp(count, 1, scenario == Scenario::Moving ? 25000 : 50000);
        const int columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(safeCount)))));
        const int rows = (safeCount + columns - 1) / columns;
        const float spacing = scenario == Scenario::Churn ? 1.65f : 1.8f;
        if (scenario == Scenario::Moving) m_movingItems.reserve(static_cast<std::size_t>(safeCount));

        for (int index = 0; index < safeCount; ++index) {
            Origo::GameObject& object = AppendSceneObject(scene, "PerfLab:Cube:" + std::to_string(index));
            object.mobility = scenario == Scenario::Static ? Origo::ObjectMobility::Static : Origo::ObjectMobility::Movable;
            if (scenario == Scenario::Churn) {
                object.transform.x = RandomRange(-static_cast<float>(columns) * 0.9f, static_cast<float>(columns) * 0.9f);
                object.transform.y = RandomRange(-static_cast<float>(rows) * 0.9f, static_cast<float>(rows) * 0.9f);
                object.transform.z = RandomRange(0.25f, 18.0f);
                object.transform.rx = RandomRange(-180.0f, 180.0f);
                object.transform.ry = RandomRange(-180.0f, 180.0f);
                object.transform.rz = RandomRange(-180.0f, 180.0f);
                const float scale = RandomRange(0.25f, 1.2f);
                object.transform.sx = scale;
                object.transform.sy = scale;
                object.transform.sz = scale;
            } else {
                const int column = index % columns, row = index / columns;
                object.transform.x = (static_cast<float>(column) - static_cast<float>(columns - 1) * 0.5f) * spacing;
                object.transform.y = (static_cast<float>(row) - static_cast<float>(rows - 1) * 0.5f) * spacing;
                object.transform.z = 0.45f;
                object.transform.sx = object.transform.sy = object.transform.sz = 0.7f;
            }
            ConfigureRenderable(object, false, false);
            if (scenario == Scenario::Moving)
                m_movingItems.push_back({object.id, object.transform.z, object.transform.rz,
                    static_cast<float>(index) * 0.173f});
            ++m_liveObjectCount;
        }
    }

    void AppendInstancePopulation(Origo::Scene& scene, int count) {
        const int safeCount = std::clamp(count, 100, 50000);
        Origo::GameObject& object = AppendSceneObject(scene, "PerfLab:InstancedMesh");
        object.mobility = Origo::ObjectMobility::Static;
        ConfigureRenderable(object, false, false);
        object.hasInstancedStaticMesh = true;
        object.instancedStaticMesh.enabled = true;
        object.instancedStaticMesh.cullDistance = kBenchmarkDrawDistance;
        object.instancedStaticMesh.instances.reserve(static_cast<std::size_t>(safeCount));
        const int columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(safeCount)))));
        const int rows = (safeCount + columns - 1) / columns;
        constexpr float spacing = 1.8f;
        constexpr float scale = 0.7f;
        for (int index = 0; index < safeCount; ++index) {
            const int column = index % columns, row = index / columns;
            const float x = (static_cast<float>(column) - static_cast<float>(columns - 1) * 0.5f) * spacing;
            const float y = (static_cast<float>(row) - static_cast<float>(rows - 1) * 0.5f) * spacing;
            object.instancedStaticMesh.instances.emplace_back(x, y, 0.45f, 0, 0, 0, scale, scale, scale);
        }
        m_liveObjectCount = safeCount;
    }

    void AppendPhysicsFloor(Origo::Scene& scene, int count) {
        const float extent = std::max(30.0f, std::sqrt(static_cast<float>(count)) * 1.7f + 12.0f);
        Origo::GameObject& floor = AppendSceneObject(scene, "PerfLab:Floor");
        floor.mobility = Origo::ObjectMobility::Static;
        floor.transform.z = -0.55f;
        floor.transform.sx = extent;
        floor.transform.sy = extent;
        floor.transform.sz = 0.5f;
        ConfigureRenderable(floor, m_physicsCastShadows, true);
        floor.hasCollider = true;
        floor.collider.shape = Origo::ColliderShape::Box;
        floor.collider.sizeX = floor.collider.sizeY = floor.collider.sizeZ = 1.0f;
        floor.collider.friction = 0.8f;
        floor.collider.restitution = 0.05f;
    }

    void AppendPhysicsPopulation(Origo::Scene& scene, int count) {
        const int safeCount = std::clamp(count, 1, 1500);
        const float spread = std::max(4.0f, std::sqrt(static_cast<float>(safeCount)) * 0.56f);
        for (int index = 0; index < safeCount; ++index) {
            Origo::GameObject& object = AppendSceneObject(scene, "PerfLab:Body:" + std::to_string(index));
            const bool sphere = m_physicsShapeMode == 1 || (m_physicsShapeMode == 2 && RandomBool(0.62f));
            object.mobility = Origo::ObjectMobility::Movable;
            object.transform.x = RandomRange(-spread, spread);
            object.transform.y = RandomRange(-spread, spread);
            object.transform.z = RandomRange(2.0f, 20.0f) + (index % 4) * 0.35f;
            object.transform.rx = RandomRange(-180.0f, 180.0f);
            object.transform.ry = RandomRange(-180.0f, 180.0f);
            object.transform.rz = RandomRange(-180.0f, 180.0f);
            const float scale = sphere ? RandomRange(0.65f, 1.0f) : RandomRange(0.7f, 1.0f);
            object.transform.sx = object.transform.sy = object.transform.sz = scale;
            ConfigureRenderable(object, m_physicsCastShadows, true);
            if (sphere && m_sphereMesh) {
                object.meshRenderer.mesh = m_sphereMesh;
                object.meshRenderer.meshGuid = kSphereGuid;
            }

            object.hasCollider = true;
            object.collider.shape = sphere ? Origo::ColliderShape::Sphere : Origo::ColliderShape::Box;
            object.collider.radius = sphere ? 1.0f : 0.5f;
            object.collider.sizeX = object.collider.sizeY = object.collider.sizeZ = 1.0f;
            object.collider.friction = sphere ? 0.22f : 0.38f;
            object.collider.restitution = m_physicsRestitution;
            object.hasRigidbody = true;
            object.rigidbody.enabled = true;
            object.rigidbody.isKinematic = false;
            object.rigidbody.useGravity = true;
            object.rigidbody.allowSleeping = false;
            object.rigidbody.continuousCollision = true;
            object.rigidbody.mass = sphere ? RandomRange(0.4f, 1.5f) : RandomRange(0.8f, 3.0f);
            object.rigidbody.linearDamping = 0.015f;
            object.rigidbody.angularDamping = 0.025f;
            object.rigidbody.velocityX = RandomRange(-m_physicsImpulse, m_physicsImpulse);
            object.rigidbody.velocityY = RandomRange(-m_physicsImpulse, m_physicsImpulse);
            object.rigidbody.velocityZ = RandomRange(-0.5f, m_physicsImpulse * 0.6f);
            object.rigidbody.angularVelocityX = RandomRange(-m_physicsImpulse, m_physicsImpulse);
            object.rigidbody.angularVelocityY = RandomRange(-m_physicsImpulse, m_physicsImpulse);
            object.rigidbody.angularVelocityZ = RandomRange(-m_physicsImpulse, m_physicsImpulse);
            ++m_liveObjectCount;
        }
    }

    void AppendLightingPopulation(Origo::Scene& scene, int count) {
        const float columnsForBounds = std::ceil(std::sqrt(static_cast<float>(std::max(1, count))));
        const float extent = std::max(40.0f, 2.0f * ((columnsForBounds - 1.0f) * 3.5f + 12.0f));
        Origo::GameObject& floor = AppendSceneObject(scene, "PerfLab:LightReceiver");
        floor.mobility = Origo::ObjectMobility::Static;
        floor.transform.z = -0.3f;
        floor.transform.sx = extent;
        floor.transform.sy = extent;
        floor.transform.sz = 0.25f;
        ConfigureRenderable(floor, true, true);
        ++m_liveObjectCount;

        const int columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(std::max(1, count))))));
        const int rows = (std::max(1, count) + columns - 1) / columns;
        for (int i = 0; i < count; ++i) {
            Origo::GameObject& lightObject = AppendSceneObject(scene, "PerfLab:PointLight:" + std::to_string(i));
            const int column = i % columns, row = i / columns;
            lightObject.transform.x = (static_cast<float>(column) - static_cast<float>(columns - 1) * 0.5f) * 7.0f;
            lightObject.transform.y = (static_cast<float>(row) - static_cast<float>(rows - 1) * 0.5f) * 7.0f;
            lightObject.transform.z = 4.0f + RandomRange(-0.5f, 1.5f);
            lightObject.hasLight = true;
            lightObject.light.type = Origo::LightComponent::Type::Point;
            lightObject.light.r = RandomRange(0.35f, 1.0f);
            lightObject.light.g = RandomRange(0.35f, 1.0f);
            lightObject.light.b = RandomRange(0.35f, 1.0f);
            lightObject.light.intensity = RandomRange(1.5f, 5.0f);
            lightObject.light.range = 14.0f;
            lightObject.light.castShadows = m_lightShadows;
            ++m_liveObjectCount;
        }
    }

    void UpdateMovingObjects(Origo::Scene& scene) {
        const float time = m_elapsedSeconds;
        for (const MovingItem& item : m_movingItems) {
            Origo::GameObject* object = scene.FindObject(item.id);
            if (!object) continue;
            object->transform.z = item.baseZ + std::sin(time * 1.3f + item.phase) * 0.65f;
            object->transform.rz = std::fmod(item.baseRotation + time * 48.0f + item.phase * 57.29578f, 360.0f);
        }
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
        if (m_particleAccumulator < m_particleInterval) return;
        m_particleAccumulator = std::fmod(m_particleAccumulator, m_particleInterval);
        const int burstCount = std::max(1, static_cast<int>(
            static_cast<float>(m_targetCount) * m_particleInterval / 3.0f));
        SpawnParticleBurst(scene, static_cast<std::uint32_t>(burstCount));
    }

    void FitCamera(Origo::Scene& scene) {
        if (!m_cameraId) return;
        float halfX = 12.0f, halfY = 12.0f, halfZ = 4.0f;
        if (m_mode == Scenario::Static || m_mode == Scenario::Moving || m_mode == Scenario::Instances) {
            const float n = static_cast<float>(std::max(1, m_targetCount));
            const float spacing = 1.8f;
            const float columns = std::ceil(std::sqrt(n));
            const float rows = std::ceil(n / columns);
            halfX = std::max(2.0f, (columns - 1.0f) * spacing * 0.5f + 1.5f);
            halfY = std::max(2.0f, (rows - 1.0f) * spacing * 0.5f + 1.5f);
            halfZ = m_mode == Scenario::Moving ? 2.0f : 1.5f;
        } else if (m_mode == Scenario::Physics) {
            const float spread = std::max(4.0f, std::sqrt(static_cast<float>(m_targetCount)) * 0.56f);
            halfX = halfY = spread + 2.0f;
            halfZ = 12.0f;
        } else if (m_mode == Scenario::Lights) {
            const float columns = std::ceil(std::sqrt(static_cast<float>(std::max(1, m_targetCount))));
            const float rows = std::ceil(static_cast<float>(std::max(1, m_targetCount)) / columns);
            halfX = (columns - 1.0f) * 3.5f + 12.0f;
            halfY = (rows - 1.0f) * 3.5f + 12.0f;
            halfZ = 5.0f;
        } else if (m_mode == Scenario::Churn) {
            const float columns = std::ceil(std::sqrt(static_cast<float>(std::max(1, m_targetCount))));
            halfX = halfY = columns * 1.2f + 4.0f;
            halfZ = 9.0f;
        }

        const float radius = std::sqrt(halfX * halfX + halfY * halfY + halfZ * halfZ);
        const float fovRadians = std::clamp(m_cameraFov, 35.0f, 100.0f) * 0.01745329252f;
        const float safeSin = std::max(0.2f, std::sin(fovRadians * 0.5f));
        m_cameraDistance = std::max(35.0f, radius / safeSin * m_cameraMargin);
        if (m_mode == Scenario::Physics) m_cameraTarget = {0.0f, 0.0f, 7.0f};
        else if (m_mode == Scenario::Lights) m_cameraTarget = {0.0f, 0.0f, 1.5f};
        else if (m_mode == Scenario::Churn) m_cameraTarget = {0.0f, 0.0f, 8.0f};
        else m_cameraTarget = {0.0f, 0.0f, 0.0f};

        m_cameraDirection = Normalize({-0.55f, -0.65f, 0.52f});
        m_cameraHomePosition = {
            m_cameraTarget.x + m_cameraDirection.x * m_cameraDistance,
            m_cameraTarget.y + m_cameraDirection.y * m_cameraDistance,
            m_cameraTarget.z + m_cameraDirection.z * m_cameraDistance
        };
        m_cameraHomeForward = {-m_cameraDirection.x, -m_cameraDirection.y, -m_cameraDirection.z};
        scene.SetRuntimeCameraView(m_cameraId, m_cameraHomePosition, m_cameraHomeForward, Origo::WorldAxes::Up);
        m_cameraWasSweeping = false;
    }

    void UpdateCamera(Origo::Scene& scene, float dt) {
        if (!m_cameraId) return;
        if (m_cameraSweep) {
            m_sweepAngle += dt * 0.18f;
            const float radius = std::max(35.0f, m_cameraDistance);
            const Origo::Vec3 position{
                m_cameraTarget.x + std::cos(m_sweepAngle) * radius * 0.83f,
                m_cameraTarget.y + std::sin(m_sweepAngle) * radius * 0.83f,
                m_cameraTarget.z + radius * 0.36f
            };
            const Origo::Vec3 forward = Normalize({
                m_cameraTarget.x - position.x,
                m_cameraTarget.y - position.y,
                m_cameraTarget.z - position.z
            });
            scene.SetRuntimeCameraView(m_cameraId, position, forward, Origo::WorldAxes::Up);
            m_cameraWasSweeping = true;
        } else if (m_cameraWasSweeping) {
            scene.SetRuntimeCameraView(m_cameraId, m_cameraHomePosition, m_cameraHomeForward, Origo::WorldAxes::Up);
            m_cameraWasSweeping = false;
        }
    }

    int CountStep() const {
        switch (m_mode) {
        case Scenario::Static: return 1000;
        case Scenario::Instances: return 1000;
        case Scenario::Moving: return 500;
        case Scenario::Physics: return 50;
        case Scenario::Particles: return 1000;
        case Scenario::Lights: return 8;
        case Scenario::Churn: return 100;
        case Scenario::Empty: return 0;
        }
        return 1000;
    }

    void AdjustCount(Origo::Scene& scene, int direction) {
        const int step = CountStep();
        if (!step || m_mode == Scenario::Empty) return;
        const int oldCount = m_targetCount;
        m_targetCount += step * direction;
        ClampCount();
        if (m_targetCount != oldCount) RebuildPopulation(scene, m_mode, m_targetCount);
    }

    void DrawHud(Origo::Scene& scene) {
        scene.UIPanel("PerfLab.Panel",
            {0.015f, 0.015f, 0.385f, 0.735f},
            {0.025f, 0.035f, 0.055f, 0.88f});

        char status[256]{};
        char timing[256]{};
        const char* populationLabel = m_mode == Scenario::Particles ? "TARGET" :
            (m_mode == Scenario::Instances ? "PLACEMENTS" : (m_mode == Scenario::Lights ? "LIGHTS" : "OBJECTS"));
        const int populationValue = m_mode == Scenario::Particles ? m_targetCount :
            (m_mode == Scenario::Lights ? m_targetCount : (m_mode == Scenario::Empty ? 0 : m_targetCount));
        std::snprintf(status, sizeof(status), "%s  |  %s %d",
            ScenarioName(m_mode), populationLabel, populationValue);
        std::snprintf(timing, sizeof(timing), "Tick delta %.2f ms | rebuild %.2f ms | %s",
            m_smoothedTickMs, m_lastBuildMs, m_cameraSweep ? "camera sweep" : "auto-fit camera");

        scene.UIText("PerfLab.Title", "ORIGO PERFLAB", 0.03f, 0.04f, 0.8f,
            {0.45f, 0.85f, 1.0f, 1.0f});
        scene.UIText("PerfLab.Status", status, 0.03f, 0.075f, 0.48f);
        scene.UIText("PerfLab.Timing", timing, 0.03f, 0.105f, 0.4f);
        scene.UIText("PerfLab.Hint", "Edit PerfLab Settings on the authored scene object in the Inspector.", 0.03f, 0.132f, 0.34f,
            {0.78f, 0.82f, 0.9f, 1.0f});

        if (scene.UIButton("PerfLab.Static1K", "OBJECTS CONFIG", {0.03f, 0.16f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Static, m_gameObjectCount);
        if (scene.UIButton("PerfLab.Static5K", "OBJECTS 5K", {0.205f, 0.16f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Static, 5000);
        if (scene.UIButton("PerfLab.Static10K", "OBJECTS 10K", {0.03f, 0.212f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Static, 10000);
        if (scene.UIButton("PerfLab.Static25K", "OBJECTS 25K", {0.205f, 0.212f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Static, 25000);
        if (scene.UIButton("PerfLab.Instances10K", "ISM 10K", {0.03f, 0.264f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Instances, 10000);
        if (scene.UIButton("PerfLab.Instances25K", "ISM 25K", {0.205f, 0.264f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Instances, 25000);
        if (scene.UIButton("PerfLab.Moving1K", "MOVING CONFIG", {0.03f, 0.316f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Moving, m_movingCount);
        if (scene.UIButton("PerfLab.Moving5K", "MOVING 5K", {0.205f, 0.316f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Moving, 5000);
        if (scene.UIButton("PerfLab.Physics100", "PHYSICS 100", {0.03f, 0.368f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Physics, 100);
        if (scene.UIButton("PerfLab.Physics500", "PHYSICS 500", {0.205f, 0.368f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Physics, 500);
        if (scene.UIButton("PerfLab.Particles5K", "PARTICLES 5K", {0.03f, 0.42f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Particles, m_particleTarget);
        if (scene.UIButton("PerfLab.Burst", "BURST 5K", {0.205f, 0.42f, 0.16f, 0.044f}))
            SpawnParticleBurst(scene, 5000);
        if (scene.UIButton("PerfLab.CameraSweep", m_cameraSweep ? "CAMERA FIXED" : "CAMERA SWEEP",
            {0.03f, 0.472f, 0.16f, 0.044f}))
            m_cameraSweep = !m_cameraSweep;
        if (scene.UIButton("PerfLab.Clear", "CLEAR SCENE", {0.205f, 0.472f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Empty, 0);
        if (scene.UIButton("PerfLab.CountMinus", "COUNT -", {0.03f, 0.524f, 0.16f, 0.044f}))
            AdjustCount(scene, -1);
        if (scene.UIButton("PerfLab.CountPlus", "COUNT +", {0.205f, 0.524f, 0.16f, 0.044f}))
            AdjustCount(scene, +1);
        if (scene.UIButton("PerfLab.Lights", "LIGHTS", {0.03f, 0.576f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Lights, m_lightCount);
        if (scene.UIButton("PerfLab.Churn", "SPAWN / DESTROY", {0.205f, 0.576f, 0.16f, 0.044f}))
            RebuildPopulation(scene, Scenario::Churn, m_churnCount);
        if (scene.UIButton("PerfLab.FitCamera", "FIT CAMERA", {0.03f, 0.628f, 0.16f, 0.044f}))
            FitCamera(scene);
        if (scene.UIButton("PerfLab.LightShadows", m_lightShadows ? "LIGHT SHADOWS ON" : "LIGHT SHADOWS OFF",
            {0.205f, 0.628f, 0.16f, 0.044f})) {
            m_lightShadows = !m_lightShadows;
            if (m_mode == Scenario::Lights) RebuildPopulation(scene, Scenario::Lights, m_targetCount);
        }
    }

    Scenario m_mode = Scenario::Static;
    std::vector<MovingItem> m_movingItems;
    std::uint32_t m_cameraId = 0;
    std::uint32_t m_sphereMesh = 0;
    std::uint32_t m_rngSeed = 1337;
    std::uint32_t m_rngState = 1337;
    Origo::Vec3 m_cameraHomePosition{-55.0f, -70.0f, 37.0f};
    Origo::Vec3 m_cameraHomeForward{0.5f, 0.6f, -0.5f};
    Origo::Vec3 m_cameraDirection{-0.55f, -0.65f, 0.52f};
    Origo::Vec3 m_cameraTarget{0.0f, 0.0f, 0.0f};
    float m_cameraFov = 62.0f;
    float m_cameraDistance = 100.0f;
    float m_cameraMargin = 1.2f;
    float m_elapsedSeconds = 0.0f;
    float m_particleAccumulator = 0.0f;
    float m_churnAccumulator = 0.0f;
    float m_particleInterval = 0.2f;
    float m_churnInterval = 0.5f;
    float m_smoothedTickMs = 0.0f;
    float m_lastBuildMs = 0.0f;
    float m_sweepAngle = 0.0f;
    float m_physicsRestitution = 0.82f;
    float m_physicsImpulse = 4.0f;
    int m_gameObjectCount = 1000;
    int m_instanceCount = 10000;
    int m_movingCount = 1000;
    int m_physicsCount = 180;
    int m_physicsShapeMode = 2;
    int m_particleTarget = 5000;
    int m_lightCount = 16;
    int m_churnCount = 500;
    int m_targetCount = 1000;
    int m_liveObjectCount = 0;
    bool m_physicsCastShadows = false;
    bool m_lightShadows = false;
    bool m_haveTickSample = false;
    bool m_hudEnabled = true;
    bool m_cameraSweep = false;
    bool m_cameraWasSweeping = false;
    bool m_autoFit = true;
};
