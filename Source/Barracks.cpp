#include "Barracks.h"
#include "GimmicManager.h"
#include "CollisionManager.h"
#include "EnemyManager.h"


Barracks::Barracks()
{
	//基礎設定
	hp = 10.0f;
	spawn_interval = 5.0f;
	spawn_timer = 0.0f;
	current_enemy_count = 0;
	spawn_positon.z += 3.0f;

	//当たり判定の種類設定
	collider = std::make_unique<OBB>();
	collider->type = ColliderType::OBB;
	collider->owner = this;
	obb = static_cast<OBB*>(collider.get());

	CollisionManager::Instance().AddObject(this);
}

//ギミック更新処理
void Barracks::Update(float elapsedTime)
{
	if (hp <= 0.0f)
	{
		CollisionManager::Instance().Remove(this);
		GimmicManager::Instance().Remove(this);
		return;
	}

	//スポーン位置設定
	spawn_positon.x += position.x;
	spawn_positon.y += position.y;
	spawn_positon.z += position.z;

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

}

//デバッグ表示
void Barracks::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{

}

//衝突処理
void Barracks::OnCollision(GameObject* object)
{
	if (object->type == Type::PlayerAttack)hp -= 2.5f;
}

REGISTER_GAMEOBJECT(Barracks);