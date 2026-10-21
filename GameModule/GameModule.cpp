// AUTO-GENERATED-STYLE ORIGO gameplay registry. The editor may regenerate this file
// from ORIGO_GAME_MODE_CLASS annotations in Assets/Scripts.
#include "ScriptSystem.h"
#include "PerfLabGameMode.h"

namespace {
Origo::GameMode* CreatePerfLabGameMode() { return new PerfLabGameMode(); }
void DestroyPerfLabGameMode(Origo::GameMode* mode) { delete static_cast<PerfLabGameMode*>(mode); }

const Origo::GameModeModuleEntry kGameModes[] = {
    {"PerfLabGameMode", &CreatePerfLabGameMode, &DestroyPerfLabGameMode},
};
const Origo::ScriptModuleApi kApi{
    Origo::kScriptModuleAbiVersion,
    0u,
    nullptr,
    static_cast<std::uint32_t>(sizeof(kGameModes) / sizeof(kGameModes[0])),
    kGameModes
};
}

ORIGO_SCRIPT_MODULE_EXPORT const Origo::ScriptModuleApi* OrigoGetScriptModuleApi() {
    return &kApi;
}
