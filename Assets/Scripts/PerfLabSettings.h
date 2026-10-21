#pragma once
#include "ScriptSystem.h"

// Place this component on the authored "PerfLab Settings" object. These values
// are reflected into the ORIGO Inspector and serialized with the test map.
ORIGO_SCRIPT_CLASS(PerfLabSettings)
class PerfLabSettings final : public Origo::Script {
public:
    ORIGO_PROPERTY_INT(GameObjectCount, 1000, 100, 50000, 100);
    ORIGO_PROPERTY_INT(InstanceCount, 10000, 100, 50000, 100);
    ORIGO_PROPERTY_INT(MovingCount, 1000, 100, 25000, 100);
    ORIGO_PROPERTY_INT(PhysicsCount, 180, 1, 1500, 10);
    // 0 = boxes, 1 = balls, 2 = mixed boxes and balls.
    ORIGO_PROPERTY_INT(PhysicsShapeMode, 2, 0, 2, 1);
    ORIGO_PROPERTY_INT(PhysicsSeed, 1337, 0, 999999, 1);
    ORIGO_PROPERTY_FLOAT(PhysicsRestitution, 0.82f, 0.0f, 1.0f, 0.05f);
    ORIGO_PROPERTY_FLOAT(PhysicsImpulse, 4.0f, 0.0f, 30.0f, 0.5f);
    ORIGO_PROPERTY_INT(ParticleTarget, 5000, 100, 50000, 100);
    ORIGO_PROPERTY_INT(LightCount, 16, 0, 128, 1);
    ORIGO_PROPERTY_BOOL(LightShadows, false);
    ORIGO_PROPERTY_INT(ChurnCount, 500, 10, 5000, 10);
    ORIGO_PROPERTY_FLOAT(CameraMargin, 1.2f, 1.0f, 2.5f, 0.05f);
};
