#pragma once

#include "System/Model.h"
#include "Projectile.h"

// ’¼i’eŠÛ
class ProjectileStraite : public Projectile
{
public:
	//ProjectileStraite();
	ProjectileStraite(ProjectileManager* manager);
	~ProjectileStraite() override;

	// XVˆ—
	void Update(float elapsedTime) override;

	// •`‰æˆ—
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;

	// ”­Ë
	void Launch(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& position);

private:
	Model* model = nullptr;
	float		speed = 10.0f;
	float lifeTimer = 3.0f;
	int damage = 10; // b’è
};
