#pragma once
#include "Gimmic_BreakWall.h"
class Well :
    public Gimmic_BreakWall
{
public:
    Well();
    void Update(float elapsedTime)override;
    void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
private:
    CylinderCollider* cylinder;

};

