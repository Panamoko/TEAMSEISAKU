#pragma once

#include "System/ModelRenderer.h"
#include "System/ShapeRenderer.h"

//前方宣言
class ProjectileManager;//相互インクルードしないように前方宣言する


// 弾丸
class Projectile
{
public:
	//Projectile() {};
	Projectile(ProjectileManager* manager) ;
	virtual ~Projectile() {}

	// 更新処理
	virtual void Update(float elapsedTime) = 0;

	// 描画処理
	virtual void Render(const RenderContext& rc, ModelRenderer* renderer) = 0;

	// デバッグプリミティブ描画
	virtual void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

	// 位置取得
	const DirectX::XMFLOAT3& GetPosition() const { return position; }

	// 方向取得
	const DirectX::XMFLOAT3& GetDirection() const { return direction; }

	// スケール取得
	const DirectX::XMFLOAT3& GetScale() const { return scale; }

	void SetDamage(int d) { damage = d; }
	int  GetDamage() const { return damage; }

	//破棄
	void Destroy();

	//半径取得
	float GetRadius() const { return radius; }

protected:
	// 行列更新処理
	void UpdateTransform();

protected:
	DirectX::XMFLOAT3		position = { 0, 0, 0 };
	DirectX::XMFLOAT3		direction = { 0, 0, 1 };
	DirectX::XMFLOAT3		scale = { 1, 1, 1 };
	DirectX::XMFLOAT4X4		transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	ProjectileManager* manager = nullptr;

	float radius = 0.5f;

	int damage = 10;
};
