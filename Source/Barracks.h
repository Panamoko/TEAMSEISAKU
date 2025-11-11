#pragma once
#include "GimmicBase.h"
#include "Collider.h"
#include "EnemySlime.h"

class Barracks :public GimmicBase
{
public:
	Barracks();
	void Update(float elapsedTime)override;//ギミック更新処理
	//デバッグ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
	void OnCollision(GameObject* object)override;//衝突処理

private:
	OBB* obb;
	DirectX::XMFLOAT3 spawn_positon;		//出現場所
	float spawn_interval;					//再出現時間
	float spawn_timer;						//経過時間
	const int max_enemy_count = 5;			//最大出現数
	int current_enemy_count;				//現在の出現数
	float hp;								//耐久度

	std::shared_ptr<EnemySlime> enemys;		//敵
	std::vector<std::shared_ptr<Enemy>> spawned_enemies;	//兵舎用の敵管理リスト
};

