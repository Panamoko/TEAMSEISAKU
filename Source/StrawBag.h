#pragma once
#include "GimmicBase.h"
class StrawBag :
    public GimmicBase
{
public:
    StrawBag();
    void Update(float elapsedTime)override;
    void OnCollision(GameObject* objects) override;
    void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
    void Render(const RenderContext& rc, ModelRenderer* renderer) override;
private:
    OBB* obb;
};

