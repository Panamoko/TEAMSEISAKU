#include "Fence.h"
#include "System/RenderContext.h"
#include "System/ShapeRenderer.h"
#include "System/ModelRenderer.h"
using namespace DirectX;

void Fence::Render(const RenderContext& rc, ModelRenderer* renderer) {
    if (!IsAlive() || !model) return;
    XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
    XMMATRIX R = XMMatrixRotationRollPitchYaw(0.0f, angleY, 0.0f);
    XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
    XMFLOAT4X4 transform;
    XMStoreFloat4x4(&transform, S * R * T);
    renderer->Render(rc, transform, model, ShaderId::Lambert);
}

void Fence::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) {
    if (!renderer || !IsAlive()) return;
    // いったん円柱で近似（後で長方形コリジョンに置換）
    renderer->RenderBox(rc, position, { 0, angleY, 0 }, { halfX, halfY, halfZ }, { 0,1,1,1 });
}
