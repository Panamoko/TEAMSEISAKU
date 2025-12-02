#include "Yagura.h"
#include "GimmicManager.h"
#include "Player.h"
#include "Factory.h"
#include "EnemyManager.h"
#include "GameObjectManager.h"

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

	// 生成フラグ初期化
	isTurretSpawned = false;
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

		// タレットも破壊
		if (turret)
		{
			turret->Destroy(); // または、地面に落とす処理を入れても良い
			turret = nullptr;
		}
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
	float halfHeight = 50.0f * scale.y;
	obb->half = DirectX::XMFLOAT3(20.0f * scale.x, halfHeight, 20.0f * scale.z);

	//OBB設定
	obb->center = position;
	obb->center.x -= 0.1f;

	// タレットのスポーン処理
	if (!isTurretSpawned)
	{
		// 櫓のてっぺんの座標を計算
		// 櫓のY座標(中心) + OBBの高さ半分 + タレットの足元の補正(少し浮かせる等)
		DirectX::XMFLOAT3 spawnPos = position;
		spawnPos.y += halfHeight + 2.0f; // +1.0fはタレットの高さ半分など微調整用

		// タレット生成
		turret = std::make_shared<EnemySlimeTurret>();
		turret->SetPosition(spawnPos);

		// 櫓の回転に合わせてタレットも向きを合わせるなら
		turret->SetAngle({ 0, angle.y, 0 });

		// 縄張りを設定（固定砲台なので、自分の位置を中心に索敵）
		turret->SetTerritory(spawnPos, 15.0f);

		// マネージャーに登録 (Barracksの実装に合わせる)
		EnemyManager::Instance().Register(turret);
		GameObjectManager::Instance().AddObject(turret);

		isTurretSpawned = true;
	}

	if (turret && !turret->IsDestroyRequested())
	{
		DirectX::XMFLOAT3 currentTop = position;
		// 櫓のY座標(中心) + 高さ半分 + タレット足元補正
		currentTop.y += halfHeight + 2.0f;

		// 位置を更新（X, Zは櫓と同じ、Yは計算した高さ）
		turret->SetPosition(currentTop);

		// ついでに重力による落下速度もリセットしておくと安心
		// (Characterクラスに SetVelocity のような関数があれば使う)
		turret->SetVelocity({ 0.0f, 0.0f, 0.0f });
	}

	// タレットが倒された場合のポインタクリア処理
	if (turret && turret->IsDestroyRequested())
	{
		turret = nullptr;
	}
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