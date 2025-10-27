#pragma once

#include "Projectile.h"

// 追尾弾丸
class ProjectileHoming : public Projectile
{
public:
	ProjectileHoming(ProjectileManager* manager);
	~ProjectileHoming() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;

	// 発射
	void Launch(const DirectX::XMFLOAT3& direction, 
				const DirectX::XMFLOAT3& position, 
				const DirectX::XMFLOAT3& target);//ここで設定するターゲットに向かって追尾する

private:
	Model* model = nullptr;
	DirectX::XMFLOAT3	target = { 0, 0, 0 };
	float				moveSpeed = 10.0f;
	float				turnSpeed = DirectX::XMConvertToRadians(180);
	float				lifeTimer = 3.0f;
};

