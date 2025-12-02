#pragma once
#include "Projectile.h"
#include "System/Model.h"

class Character;

// タレット専用：イージングで誘導力が減衰する弾
class ProjectileTurret : public Projectile
{
public:
    ProjectileTurret(ProjectileManager* manager);
    ~ProjectileTurret() override;

    void Update(float elapsedTime) override;
    void Render(const RenderContext& rc, ModelRenderer* renderer) override;

    // ターゲットを指定して発射
    void Launch(const DirectX::XMFLOAT3& direction,
        const DirectX::XMFLOAT3& position,
        Character* target);

private:
    Model* model = nullptr;
    Character* target = nullptr; // 狙う対象

    float speed = 8.0f;          // 弾速
    float maxTurnSpeed = 3.0f;   // 最大旋回速度（ラジアン/秒）
    float lifeTimer = 5.0f;      // 生存時間
    float maxLife = 5.0f;        // 初期生存時間（イージング計算用）
};