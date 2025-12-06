#include "ProjectileTurret.h"
#include "ModelManager.h"
#include "Collider.h"
#include "Character.h"
#include <cmath>
#include <algorithm>

ProjectileTurret::ProjectileTurret(ProjectileManager* manager)
    : Projectile(manager)
{
    // モデル読み込み
    auto resource = ModelManager::Instance().GetResource("Data/Model/Slime/Bullet.mdl");
    model = new Model(resource, "Data/Model/Slime/Bullet.mdl");

    // スケール・当たり判定
    scale = { 0.15f, 0.15f, 0.15f }; // 少し大きめ
    radius = 0.5f;
    type = Type::EnemyAttack;

    collider = std::make_unique<SphereCollider>();
    collider->type = ColliderType::Sphere;
    collider->owner = this;
    static_cast<SphereCollider*>(collider.get())->radius = radius;
}

ProjectileTurret::~ProjectileTurret()
{
    delete model;
}

void ProjectileTurret::Update(float elapsedTime)
{
    lifeTimer -= elapsedTime;
    if (lifeTimer <= 0.0f)
    {
        Destroy();
        return;
    }

    // 最大旋回速度
    float currentMaxTurnSpeed = 1.0f;

    // 進行度 t (0.0 -> 1.0)
    float t = 1.0f - (lifeTimer / maxLife);

    // (1.0f - t) * (1.0f - t); (開始直後がピークで、すぐに誘導しなくなる)
    float homingFactor = 1.0f - (t * t * t);

    // ターゲットが生きていれば旋回
    if (target && target->GetHealth() > 0 && homingFactor > 0.01f)
    {
        DirectX::XMFLOAT3 tgtPos = target->GetPosition();
        tgtPos.y += target->GetHeight() * 0.5f;

        float dx = tgtPos.x - position.x;
        float dy = tgtPos.y - position.y;
        float dz = tgtPos.z - position.z;

        float len = sqrtf(dx * dx + dy * dy + dz * dz);
        if (len > 0.001f)
        {
            dx /= len; dy /= len; dz /= len;

            DirectX::XMFLOAT3 currentDir = direction;

            // 旋回量を計算
            float step = currentMaxTurnSpeed * homingFactor * elapsedTime;

            currentDir.x += (dx - currentDir.x) * step;
            currentDir.y += (dy - currentDir.y) * step;
            currentDir.z += (dz - currentDir.z) * step;

            float newLen = sqrtf(currentDir.x * currentDir.x + currentDir.y * currentDir.y + currentDir.z * currentDir.z);
            if (newLen > 0.001f)
            {
                direction.x = currentDir.x / newLen;
                direction.y = currentDir.y / newLen;
                direction.z = currentDir.z / newLen;
            }
        }
    }

    // 移動
    position.x += direction.x * speed * elapsedTime;
    position.y += direction.y * speed * elapsedTime;
    position.z += direction.z * speed * elapsedTime;

    if (collider) static_cast<SphereCollider*>(collider.get())->center = position;

    UpdateTransform();
    model->UpdateTransform();
}

void ProjectileTurret::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    renderer->Render(rc, transform, model, ShaderId::Lambert);
}

void ProjectileTurret::Launch(const DirectX::XMFLOAT3& dir, const DirectX::XMFLOAT3& pos, Character* tgt)
{
    direction = dir;
    position = pos;
    target = tgt; // ターゲットを記憶

    // 初期の向きを行列に反映
    UpdateTransform();
}