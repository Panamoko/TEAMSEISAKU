#include "Barracks.h"
#include "GimmicManager.h"
#include "CollisionManager.h"
#include "EnemyManager.h"
#include "GameObjectManager.h"
#include <imgui.h>


Barracks::Barracks()
{
	//基礎設定
	hp = 10.0f;
	spawn_interval = 5.0f;
	spawn_timer = 0.0f;
	current_enemy_count = 0;
	spawn_positon.z += 3.0f;
	is_active = true;
	class_name = "Barracks";

	//当たり判定の種類設定
	collider = std::make_unique<OBB>();
	collider->type = ColliderType::OBB;
	collider->owner = this;
	obb = static_cast<OBB*>(collider.get());

	CollisionManager::Instance().AddObject(this);

	scale = { 0.04,0.04,0.04 };
}

//ギミック更新処理
void Barracks::Update(float elapsedTime)
{
	if (hp <= 0.0f)
	{
		is_active = false;
		CollisionManager::Instance().Remove(this);
		GimmicManager::Instance().Remove(this);
		return;
	}

	//スポーン位置設定
	spawn_positon.x = position.x;
	spawn_positon.y = position.y;
	spawn_positon.z = position.z;

	spawn_timer += elapsedTime;

	current_enemy_count = static_cast<int>(spawned_enemies.size());//スポーンしている敵の数を確認

	//敵のスポーン数チェックと経過時間チェック
	if (current_enemy_count < max_enemy_count && spawn_timer >= spawn_interval)
	{
		spawn_timer = 0.0f;							//経過時間をリセット

		enemys = std::make_shared<EnemySlime>();	//敵をスポーン
		enemys->SetPosition(spawn_positon);			//スポーン位置本設定
		EnemyManager::Instance().Register(enemys);	//敵を登録
		spawned_enemies.push_back(enemys);			//兵舎にも敵を登録
		GameObjectManager::Instance().AddObject(objects);
	}

	//兵舎が管理する敵の死体を整理
	spawned_enemies.erase(
		std::remove_if(
			spawned_enemies.begin(),
			spawned_enemies.end(),
			[&](const std::shared_ptr<Enemy>& enemy)
			{
				return enemy->IsDestroyRequested();
			}),
		spawned_enemies.end()
	);

	// 各軸の向きを更新（Y軸回転のみと仮定）
	float c = cosf(angle.y);
	float s = sinf(angle.y);
	obb->axis[0] = DirectX::XMFLOAT3(c, 0.0f, -s); // X軸（右）
	obb->axis[1] = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f); // Y軸（上）
	obb->axis[2] = DirectX::XMFLOAT3(s, 0.0f, c);  // Z軸（前）

	// ---- OBBサイズ設定 ----
	// モデルサイズに合わせたハーフサイズを設定
	// (モデル単位を1とした場合の半分の大きさ)
	obb->half = DirectX::XMFLOAT3(48.0f * scale.x, 70.0f * scale.y, 70.0f * scale.z);

	//OBB設定
	obb->center = position;
	obb->center.z += 0.5f;
}

//デバッグ表示
void Barracks::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	renderer->RenderBox(rc, obb->center, angle, obb->half, { 1,0,0,1 });
}

//衝突処理
void Barracks::OnCollision(GameObject* object)
{
	if (object->type == Type::PlayerAttack)hp -= 2.5f;
}

bool Barracks::OnImGui()
{
	bool changed = false;

	if (ImGui::CollapsingHeader("Barrack"))
	{
		changed |= ImGui::DragFloat("HP", &hp, 0.1f, 0.0f, 1500.0f, "%.1f");
		changed |= ImGui::DragFloat("Spawn Interval", &spawn_interval, 0.1f, 0.1f, 60.0f, "%.1f sec");
		changed |= ImGui::DragFloat("Spawn Timer", &spawn_timer, 0.1f, 0.0f, spawn_interval * 2.0f, "%.1f sec");
		changed |= ImGui::DragInt("Max Enemy Count", &max_enemy_count, 1, 0, 100);
		changed |= ImGui::DragInt("Current Enemy Count", &current_enemy_count, 1, 0, 100);

		ImGui::DragFloat3("center", &obb->center.x, 0.1f, 0.0f, 100.0f, "%.1f");
		ImGui::DragFloat3("hal", &obb->half.x, 0.1f, 0.0f, 100.0f, "%.1f");
		ImGui::DragFloat3("axis 1", &obb->axis[0].x, 0.1f, 0.0f, 100.0f, "%.1f");
		ImGui::DragFloat3("axis 2", &obb->axis[1].x, 0.1f, 0.0f, 100.0f, "%.1f");
		ImGui::DragFloat3("axis 3", &obb->axis[2].x, 0.1f, 0.0f, 100.0f, "%.1f");

	}
	return changed;
}

void Barracks::CopyUniqueMembers(const GameObject* source)
{
	const Barracks* gimmic = dynamic_cast<const Barracks*>(source);

	if (gimmic)
	{
		this->hp = gimmic->hp;
		this->max_enemy_count = gimmic->max_enemy_count;
		this->spawn_interval = gimmic->spawn_interval;
		this->spawn_timer = gimmic->spawn_timer;
	}

}

REGISTER_GAMEOBJECT(Barracks);