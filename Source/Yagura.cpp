#include "Yagura.h"
#include "GimmicManager.h"
#include "Player.h"
#include "Factory.h"

Yagura::Yagura()
{
	//当たり判定設定
	collider = std::make_unique<OBB>();
	collider->type = ColliderType::OBB;
	collider->owner = this;
	obb = static_cast<OBB*>(collider.get());
	CollisionManager::Instance().AddObject(this);

	//基礎設定
	class_name = "Yagura";
	hp = 100.0f;
	scale = { 0.1,0.1,0.1 };
}

Yagura::~Yagura()
{
}

//ギミック更新処理
void Yagura::Update(float elapsedTime)
{
	if (hp <= 0.0f)
	{
		is_active = false;
		GimmicManager::Instance().Remove(this);
		CollisionManager::Instance().Remove(this);
		return;
	}

	UpdateInvicible(elapsedTime);

	// 各軸の向きを更新（Y軸回転のみと仮定）
	DirectX::XMFLOAT3 rotation = {
	DirectX::XMConvertToRadians(angle.x),
	DirectX::XMConvertToRadians(angle.y),
	DirectX::XMConvertToRadians(angle.z)
	};
	float c = cosf(rotation.y);
	float s = sinf(rotation.y);
	obb->axis[0] = DirectX::XMFLOAT3(c, 0.0f, -s); // X軸（右）
	obb->axis[1] = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f); // Y軸（上）
	obb->axis[2] = DirectX::XMFLOAT3(s, 0.0f, c);  // Z軸（前）

	// ---- OBBサイズ設定 ----
	// モデルサイズに合わせたハーフサイズを設定
	// (モデル単位を1とした場合の半分の大きさ)
	obb->half = DirectX::XMFLOAT3(20.0f * scale.x, 70.0f * scale.y, 20.0f * scale.z);

	//OBB設定
	obb->center = position;
	obb->center.x -= 0.1f;


}

//衝突処理
void Yagura::OnCollision(GameObject* object)
{
	if (object->type == Type::PlayerAttack && invincible_timer <= 0.0f)
	{
		hp -= 10.0f;
		invincible_timer = 0.1f;
	}
}

void Yagura::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	if (invincible_timer <= 0.0f)
	{
		GameObject::Render(rc, renderer);
	}
	else
	{
		renderer->Render(rc, transform, model, ShaderId::Lambert, { 1.0f,0.0f,0.0f,1.0f });
	}
}

void Yagura::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	DirectX::XMFLOAT3 obb_center = obb->center;
	DirectX::XMFLOAT3 obb_half = obb->half;
	DirectX::XMFLOAT3 obb_axis[3] = { obb->axis[0], obb->axis[1], obb->axis[2] };
	DirectX::XMFLOAT4 debug_color = { 0.2f, 0.8f, 0.2f, 1.0f };

	renderer->RenderOBB(
		rc,
		obb_center,
		obb_half,
		obb_axis, // 3つの軸配列を渡す
		debug_color
	);
}

REGISTER_GAMEOBJECT(Yagura);