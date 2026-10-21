// ORIGO PerfLab gameplay module registry. Registry entries are kept here so the
// Settings component appears as editable reflected properties in the Inspector.
#include "ScriptSystem.h"
#include "PerfLabGameMode.h"
#include "PerfLabSettings.h"

namespace {
Origo::GameMode* CreatePerfLabGameMode() { return new PerfLabGameMode(); }
void DestroyPerfLabGameMode(Origo::GameMode* mode) { delete static_cast<PerfLabGameMode*>(mode); }

Origo::Script* CreatePerfLabSettings() { return new PerfLabSettings(); }
void DestroyPerfLabSettings(Origo::Script* script) { delete static_cast<PerfLabSettings*>(script); }

void GetGameObjectCount(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Int;
    out->intValue = self->GameObjectCount;
}
void SetGameObjectCount(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Int) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->GameObjectCount = in->intValue;
}
void GetInstanceCount(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Int;
    out->intValue = self->InstanceCount;
}
void SetInstanceCount(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Int) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->InstanceCount = in->intValue;
}
void GetMovingCount(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Int;
    out->intValue = self->MovingCount;
}
void SetMovingCount(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Int) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->MovingCount = in->intValue;
}
void GetPhysicsCount(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Int;
    out->intValue = self->PhysicsCount;
}
void SetPhysicsCount(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Int) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->PhysicsCount = in->intValue;
}
void GetPhysicsShapeMode(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Int;
    out->intValue = self->PhysicsShapeMode;
}
void SetPhysicsShapeMode(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Int) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->PhysicsShapeMode = in->intValue;
}
void GetPhysicsSeed(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Int;
    out->intValue = self->PhysicsSeed;
}
void SetPhysicsSeed(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Int) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->PhysicsSeed = in->intValue;
}
void GetPhysicsRestitution(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Float;
    out->floatValue = self->PhysicsRestitution;
}
void SetPhysicsRestitution(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Float) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->PhysicsRestitution = in->floatValue;
}
void GetPhysicsImpulse(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Float;
    out->floatValue = self->PhysicsImpulse;
}
void SetPhysicsImpulse(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Float) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->PhysicsImpulse = in->floatValue;
}
void GetParticleTarget(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Int;
    out->intValue = self->ParticleTarget;
}
void SetParticleTarget(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Int) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->ParticleTarget = in->intValue;
}
void GetLightCount(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Int;
    out->intValue = self->LightCount;
}
void SetLightCount(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Int) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->LightCount = in->intValue;
}
void GetLightShadows(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Bool;
    out->intValue = self->LightShadows ? 1 : 0;
}
void SetLightShadows(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Bool) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->LightShadows = in->intValue != 0;
}
void GetChurnCount(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Int;
    out->intValue = self->ChurnCount;
}
void SetChurnCount(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Int) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->ChurnCount = in->intValue;
}
void GetChurnIntervalSeconds(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Float;
    out->floatValue = self->ChurnIntervalSeconds;
}
void SetChurnIntervalSeconds(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Float) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->ChurnIntervalSeconds = in->floatValue;
}
void GetParticleBurstInterval(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Float;
    out->floatValue = self->ParticleBurstInterval;
}
void SetParticleBurstInterval(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Float) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->ParticleBurstInterval = in->floatValue;
}
void GetCameraMargin(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Float;
    out->floatValue = self->CameraMargin;
}
void SetCameraMargin(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Float) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->CameraMargin = in->floatValue;
}
void GetPhysicsCastShadows(const Origo::Script* base, Origo::ScriptPropertyData* out) {
    if (!base || !out) return;
    *out = {};
    const auto* self = static_cast<const PerfLabSettings*>(base);
    out->type = Origo::PropertyType::Bool;
    out->intValue = self->PhysicsCastShadows ? 1 : 0;
}
void SetPhysicsCastShadows(Origo::Script* base, const Origo::ScriptPropertyData* in) {
    if (!base || !in || in->type != Origo::PropertyType::Bool) return;
    auto* self = static_cast<PerfLabSettings*>(base);
    self->PhysicsCastShadows = in->intValue != 0;
}
const Origo::ScriptPropertyEntry kPerfLabSettingsProperties[] = {
    {"GameObjectCount", Origo::PropertyType::Int, Origo::kScriptPropertyHasRange, 100, 50000, 100, &GetGameObjectCount, &SetGameObjectCount},
    {"InstanceCount", Origo::PropertyType::Int, Origo::kScriptPropertyHasRange, 100, 50000, 100, &GetInstanceCount, &SetInstanceCount},
    {"MovingCount", Origo::PropertyType::Int, Origo::kScriptPropertyHasRange, 100, 25000, 100, &GetMovingCount, &SetMovingCount},
    {"PhysicsCount", Origo::PropertyType::Int, Origo::kScriptPropertyHasRange, 1, 1500, 10, &GetPhysicsCount, &SetPhysicsCount},
    {"PhysicsShapeMode", Origo::PropertyType::Int, Origo::kScriptPropertyHasRange, 0, 2, 1, &GetPhysicsShapeMode, &SetPhysicsShapeMode},
    {"PhysicsSeed", Origo::PropertyType::Int, Origo::kScriptPropertyHasRange, 0, 999999, 1, &GetPhysicsSeed, &SetPhysicsSeed},
    {"PhysicsRestitution", Origo::PropertyType::Float, Origo::kScriptPropertyHasRange, 0, 1, 0.05, &GetPhysicsRestitution, &SetPhysicsRestitution},
    {"PhysicsImpulse", Origo::PropertyType::Float, Origo::kScriptPropertyHasRange, 0, 30, 0.5, &GetPhysicsImpulse, &SetPhysicsImpulse},
    {"ParticleTarget", Origo::PropertyType::Int, Origo::kScriptPropertyHasRange, 100, 50000, 100, &GetParticleTarget, &SetParticleTarget},
    {"LightCount", Origo::PropertyType::Int, Origo::kScriptPropertyHasRange, 0, 128, 1, &GetLightCount, &SetLightCount},
    {"LightShadows", Origo::PropertyType::Bool, 0u, 0, 0, 0, &GetLightShadows, &SetLightShadows},
    {"ChurnCount", Origo::PropertyType::Int, Origo::kScriptPropertyHasRange, 10, 5000, 10, &GetChurnCount, &SetChurnCount},
    {"ChurnIntervalSeconds", Origo::PropertyType::Float, Origo::kScriptPropertyHasRange, 0.1, 5, 0.1, &GetChurnIntervalSeconds, &SetChurnIntervalSeconds},
    {"ParticleBurstInterval", Origo::PropertyType::Float, Origo::kScriptPropertyHasRange, 0.05, 2, 0.05, &GetParticleBurstInterval, &SetParticleBurstInterval},
    {"CameraMargin", Origo::PropertyType::Float, Origo::kScriptPropertyHasRange, 1, 2.5, 0.05, &GetCameraMargin, &SetCameraMargin},
    {"PhysicsCastShadows", Origo::PropertyType::Bool, 0u, 0, 0, 0, &GetPhysicsCastShadows, &SetPhysicsCastShadows},
};
const Origo::ScriptModuleEntry kScripts[] = {
    {"PerfLabSettings", &CreatePerfLabSettings, &DestroyPerfLabSettings,
        static_cast<std::uint32_t>(sizeof(kPerfLabSettingsProperties) / sizeof(kPerfLabSettingsProperties[0])),
        kPerfLabSettingsProperties},
};
const Origo::GameModeModuleEntry kGameModes[] = {
    {"PerfLabGameMode", &CreatePerfLabGameMode, &DestroyPerfLabGameMode},
};
const Origo::ScriptModuleApi kApi{
    Origo::kScriptModuleAbiVersion,
    static_cast<std::uint32_t>(sizeof(kScripts) / sizeof(kScripts[0])),
    kScripts,
    static_cast<std::uint32_t>(sizeof(kGameModes) / sizeof(kGameModes[0])),
    kGameModes
};
}

ORIGO_SCRIPT_MODULE_EXPORT const Origo::ScriptModuleApi* OrigoGetScriptModuleApi() {
    return &kApi;
}
