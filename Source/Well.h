#pragma once
#include "GimmicBase.h"
class Well :
    public GimmicBase
{
public:
    Well();
    void Update(float elapsedTime)override;
    void OnCollision(GameObject* objects) override;
    void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
    void Render(const RenderContext& rc, ModelRenderer* renderer) override;
private:
    CylinderCollider* cylinder;
    bool isRespawning = false; 
    float fadeInDuration = 1.5f;
    float fadeInTimer = 0.0f;
    float respawnTimer = 0.0f;

};

