#include "MathUtils.h"

#include "EnemySlime.h"
#include "ProjectileStraite.h"
#include "Player.h"
#include "AllySlime.h"
#include "Core.h"
#include "Collision.h"
#include "EnemyManager.h"
#include <cfloat>

// コンストラクタ
EnemySlime::EnemySlime(const char* modelPath)
{
	class_name = "EnemySlime";

	// ModelManager からスライムモデル取得
	uniqueModel = ModelManager::Instance().CreateUniqueInstance(modelPath);

	// 親クラス(GameObject)のポインタにもセットしておく（描画や当たり判定で使用するため）
	model = uniqueModel.get();

	// モデルが大きいのでスケーリング
	scale.x = scale.y = scale.z = 0.01f;

	//幅・高さ設定
	radius = 0.5f;
	height = 1.0f;

	type = Type::Enemy;
	collider = std::make_unique<CylinderCollider>();
	collider->type = ColliderType::Cylinder;
	collider->owner = this;
	cylinder = static_cast<CylinderCollider*>(collider.get());

	cylinder->height = height;
	cylinder->radius = radius;

	CollisionManager::Instance().AddObject(this);

	if (!objects.empty())
	{
		auto& obj = *objects[0];
		obj.dirty = true;
		if (obj.dirty)
		{
			set_position(obj.position);
			set_rotation(obj.angle);
			set_scale(obj.scale);

			obj.dirty = false;
		}
	}

	if (model) {
		animator.SetModel(model);
		animator.Play("NIC_Idle", true); // 最初はIdle
		model->UpdateTransform();
	}

	UpdateTransform();

	//徘徊ステートへ偏移
	SetWanderState();
}

// デストラクタ
EnemySlime::~EnemySlime()
{

}

// 更新処理
void EnemySlime::Update(float elapsedTime)
{
	// タイマーを減算
	targetUpdateTimer -= elapsedTime;

	cylinder->center = position;

	//ステート毎の更新処理
	switch (state)
	{
	case State::Wander:
		UpdateWanderState(elapsedTime);
		break;
	case State::Idle:
		UpdateIdleState(elapsedTime);
		break;
	case State::Attack:
		UpdateAttackState(elapsedTime);
		break;
	}

	animator.Update(elapsedTime);

	//分離行動
	if (class_name != "EnemySlimeTurret")
	{
		ApplySeparationForce(elapsedTime);
	}

	//速力処理更新
	UpdateVelocity(elapsedTime);

	//弾丸更新処理
	projectileManager.Update(elapsedTime);

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	if (!objects.empty())
	{
		auto& obj = *objects[0];

		if (obj.dirty)
		{
			set_position(obj.position);
			set_rotation(obj.angle);
			set_scale(obj.scale);

			obj.dirty = false;
		}
	}

	// オブジェクト行列を更新
	UpdateTransform();

	// モデル行列更新
	if (model) model->UpdateTransform();
}

// 描画処理
void EnemySlime::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	// モデル描画
	
	//for (auto& obj : objects)
	//{
	//	if (!obj || !obj->model) continue;
	//	renderer->Render(rc, transform, model, ShaderId::Lambert);
	//}
	

	renderer->Render(rc, transform, model, ShaderId::Lambert);

	//弾丸描画処理
	projectileManager.Render(rc, renderer);
}

//デバッグプリミティブ描画
void EnemySlime::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	//基底クラスのデバッグプリミティブ描画
	Enemy::RenderDebugPrimitive(rc, renderer);

	// 経路の可視化
	if (gridMap && !currentPath.empty())
	{
		for (size_t i = 0; i < currentPath.size() - 1; ++i)
		{
			auto nodeA = currentPath[i];
			auto nodeB = currentPath[i + 1];

			DirectX::XMFLOAT3 p1 = gridMap->GetWorldPosition(nodeA.first, nodeA.second);
			DirectX::XMFLOAT3 p2 = gridMap->GetWorldPosition(nodeB.first, nodeB.second);

			p1.y = position.y + 0.5f;
			p2.y = position.y + 0.5f;

			float dist = sqrtf(powf(p2.x - p1.x, 2) + powf(p2.z - p1.z, 2));
			DirectX::XMFLOAT3 mid = { (p1.x + p2.x) * 0.5f, p1.y, (p1.z + p2.z) * 0.5f };

			// 簡易的に点を打つだけでもOK
			renderer->RenderSphere(rc, p1, 0.1f, { 0, 1, 0, 1 });
		}
	}

	//縄張り範囲をデバッグ円柱描画
	renderer->RenderCylinder(
		rc,
		territoryOrigin,
		territoryRange,
		1.0f,
		DirectX::XMFLOAT4(0, 1, 0, 1));

	//ターゲット位置をデバッグ球描画
	renderer->RenderSphere(
		rc,
		targetPosition,
		1.0f,
		DirectX::XMFLOAT4(1, 1, 0, 1));

	//索敵範囲をデバッグ円柱描画
	renderer->RenderCylinder(
		rc,
		position,
		searchRange,
		1.0f,
		DirectX::XMFLOAT4(1, 0, 0, 1));
}

//縄張り設定
void EnemySlime::SetTerritory(const DirectX::XMFLOAT3& origin, float range)
{
	territoryOrigin = origin;
	territoryRange = range;

	if (state == State::Wander)
	{
		targetPosition = GetStrategicTargetPosition(); 
	}
}

//ターゲット位置をランダム設定
void EnemySlime::SetRandomTargerPosition()
{
	float theta = MathUtils::RandomRenge(-DirectX::XM_PI, DirectX::XM_PI);
	float range = MathUtils::RandomRenge(0.0f, territoryRange);
	targetPosition.x = territoryOrigin.x + sinf(theta) * range;
	targetPosition.y = territoryOrigin.y;
	targetPosition.z = territoryOrigin.z + cosf(theta) * range;
}

//目標地点へ移動
void EnemySlime::MoveToTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate)
{
	// デフォルトは移動しない
	float vx = 0.0f, vz = 0.0f;
	bool shouldMove = false;

	// マップがあり、移動指示がある場合
	if (gridMap && moveSpeedRate > 0.0f)
	{
		pathRecalcTimer -= elapsedTime;
		if (pathRecalcTimer <= 0.0f)
		{
			pathRecalcTimer = 0.5f;

			auto startCell = gridMap->WorldToCell(position.x, position.z);
			auto goalCell = gridMap->WorldToCell(targetPosition.x, targetPosition.z);

			// まず通常探索
			std::vector<std::pair<int, int>> rawPath = aStar.FindPath(startCell.first, startCell.second, goalCell.first, goalCell.second, *gridMap);

			// SmoothPathを通して経路を滑らかにする
			if (!rawPath.empty())
			{
				currentPath = aStar.SmoothPath(rawPath, *gridMap);
			}
			else
			{
				currentPath.clear();
			}

			pathIndex = 0;
		}

		// 経路がある場合のみ移動ベクトルを計算
		if (!currentPath.empty() && pathIndex < currentPath.size())
		{
			auto nextCell = currentPath[pathIndex];
			DirectX::XMFLOAT3 nextPos = gridMap->GetWorldPosition(nextCell.first, nextCell.second);

			float dx = nextPos.x - position.x;
			float dz = nextPos.z - position.z;
			float dist = sqrtf(dx * dx + dz * dz);

			// ノードに到達したら次へ
			if (dist < 0.5f)
			{
				pathIndex++;
			}
			else
			{
				// 移動方向設定
				vx = dx / dist;
				vz = dz / dist;
				shouldMove = true;
			}
		}
		else
		{
			// 経路が見つからない、または最後まで到達した場合はターゲットへ直進させる
			// これにより、A*が失敗しても完全に止まることを防ぎます
			float dx = targetPosition.x - position.x;
			float dz = targetPosition.z - position.z;
			float dist = sqrtf(dx * dx + dz * dz);
			if (dist > 0.0001f)
			{
				vx = dx / dist;
				vz = dz / dist;
				shouldMove = true;
			}
		}
	}
	// マップがない場合の直進処理
	else if (moveSpeedRate > 0.0f)
	{
		float dx = targetPosition.x - position.x;
		float dz = targetPosition.z - position.z;
		float dist = sqrtf(dx * dx + dz * dz);
		if (dist > 0.0001f)
		{
			vx = dx / dist;
			vz = dz / dist;
			shouldMove = true;
		}
	}

	// 移動実行
	if (shouldMove)
	{
		Move(elapsedTime, vx, vz, moveSpeed * moveSpeedRate);
		Turn(elapsedTime, vx, vz, turnSpeed * turnSpeedRate);
	}
}

//分離行動
void EnemySlime::ApplySeparationForce(float elapsedTime)
{
	DirectX::XMFLOAT3 separation = { 0, 0, 0 };
	int neighborCount = 0;
	float separationRadius = 1.5f; // この半径内の味方から離れる（スライムの大きさより少し大きめに）

	EnemyManager& em = EnemyManager::Instance();
	int count = em.GetEnemyCount();

	for (int i = 0; i < count; ++i)
	{
		auto other = em.GetEnemy(i);
		// 自分自身や削除予定の敵は無視
		if (!other || other.get() == this || other->IsDestroyRequested()) continue;

		// 距離チェック (XZ平面)
		float dx = position.x - other->GetPosition().x;
		float dz = position.z - other->GetPosition().z;
		float distSq = dx * dx + dz * dz;

		// 近くにいる場合
		if (distSq < separationRadius * separationRadius && distSq > 0.0001f)
		{
			float dist = sqrtf(distSq);

			// 近ければ近いほど強く反発させる重み付け
			float strength = (separationRadius - dist) / dist;

			separation.x += dx * strength;
			separation.z += dz * strength;
			neighborCount++;
		}
	}

	// 力の適用
	if (neighborCount > 0)
	{
		// 平均化
		separation.x /= neighborCount;
		separation.z /= neighborCount;

		float forceStrength = 5.0f; // 反発力の強さ（調整パラメータ）

		// 現在の速度ベクトルに直接加算して軌道をずらす
		velocity.x += separation.x * forceStrength * elapsedTime;
		velocity.z += separation.z * forceStrength * elapsedTime;
	}
}

// 戦略的な徘徊地点の計算
DirectX::XMFLOAT3 EnemySlime::GetStrategicTargetPosition()
{
	Core* core = Core::Instance();

	// ランダム移動用のラムダ式（選抜漏れやコア無し時用）
	auto GetRandomPos = [&]() -> DirectX::XMFLOAT3 {
		float theta = MathUtils::RandomRenge(-DirectX::XM_PI, DirectX::XM_PI);
		float range = MathUtils::RandomRenge(0.0f, territoryRange);
		return {
			territoryOrigin.x + sinf(theta) * range,
			territoryOrigin.y,
			territoryOrigin.z + cosf(theta) * range
		};
		};

	// コアが無い、または壊れている場合はランダム移動
	if (!core || core->GetHP() <= 0.0f)
	{
		return GetRandomPos();
	}

	// 1. 8分割エリアのカウンター (0～7: 45度刻み)
	int sectorCounts[8] = { 0 };
	DirectX::XMFLOAT3 corePos = core->position;

	// カウント用ラムダ式 (8分割)
	auto CountUnit = [&](const DirectX::XMFLOAT3& pos)
		{
			float dx = pos.x - corePos.x;
			float dz = pos.z - corePos.z;
			// atan2 で角度(-PI ~ PI)を計算
			float angle = atan2f(dx, dz); // (x, z) の順で入れると 0時方向=0, 時計回り
			// 負の値を正に補正 (0 ~ 2PI)
			if (angle < 0.0f) angle += DirectX::XM_2PI;

			// 45度(PI/4)ごとに分割
			int sector = static_cast<int>(angle / (DirectX::XM_PI / 4.0f));
			// 念のため範囲制限
			sector = std::clamp(sector, 0, 7);

			sectorCounts[sector]++;
		};

	// 全プレイヤーと全味方をカウント
	const auto& players = Player::GetAllPlayers();
	for (const auto* p : players) { if (p && p->GetHealth() > 0) CountUnit(p->GetPosition()); }
	const auto& allies = AllySlime::GetAllAllies();
	for (const auto* a : allies) { if (a && a->GetHealth() > 0) CountUnit(a->GetPosition()); }

	// 最も数が多いセクターを探す
	int maxSector = 0;
	int maxCount = -1;
	for (int i = 0; i < 8; ++i)
	{
		if (sectorCounts[i] > maxCount)
		{
			maxCount = sectorCounts[i];
			maxSector = i;
		}
	}

	// 誰もいない場合はランダム
	if (maxCount <= 0)
	{
		return GetRandomPos();
	}

	// 2. ターゲット地点（激戦区の中心）を決定
	// セクターの中心角度
	float targetAngle = maxSector * (DirectX::XM_PI / 4.0f) + (DirectX::XM_PI / 8.0f);
	float distFromCore = 15.0f; // コアから少し離れた位置（調整可）

	DirectX::XMFLOAT3 strategicPos;
	strategicPos.x = corePos.x + sinf(targetAngle) * distFromCore;
	strategicPos.y = position.y;
	strategicPos.z = corePos.z + cosf(targetAngle) * distFromCore;

	// 3. 自分はこの地点に近い「選抜4体」に含まれるか？

	// 比較用の構造体
	struct Candidate {
		Enemy* enemy;
		float distSq;
	};
	std::vector<Candidate> candidates;

	EnemyManager& em = EnemyManager::Instance();
	int enemyCount = em.GetEnemyCount();

	// 全エネミーとの距離をリスト化
	for (int i = 0; i < enemyCount; ++i)
	{
		auto enemy = em.GetEnemy(i);
		// 自分と同じ種類（スライム系）で、生きているやつだけ候補にする
		if (!enemy || enemy->IsDestroyRequested()) continue;

		// 激戦区までの距離
		float dx = enemy->GetPosition().x - strategicPos.x;
		float dz = enemy->GetPosition().z - strategicPos.z;
		float d2 = dx * dx + dz * dz;

		candidates.push_back({ enemy.get(), d2 });
	}

	// 距離が近い順にソート
	std::sort(candidates.begin(), candidates.end(),
		[](const Candidate& a, const Candidate& b) {
			return a.distSq < b.distSq;
		});

	// 上位4体に入っているかチェック
	bool isElite = false;
	int limit = 4;
	for (int i = 0; i < (int)candidates.size(); ++i)
	{
		if (i >= limit) break; // 4位以降はループ終了

		// 自分がリストの中にいたら当選
		if (candidates[i].enemy == this)
		{
			isElite = true;
			break;
		}
	}

	// 選抜されたエリートなら激戦区へ、そうでなければランダム移動
	if (isElite)
	{
		// 密集しすぎないよう、目的地を少し散らす
		float rX = MathUtils::RandomRenge(-2.0f, 2.0f);
		float rZ = MathUtils::RandomRenge(-2.0f, 2.0f);
		strategicPos.x += rX;
		strategicPos.z += rZ;
		return strategicPos;
	}
	else
	{
		return GetRandomPos();
	}
}

//徘徊ステートへ偏移
void EnemySlime::SetWanderState()
{
	state = State::Wander;

	//目標地点設定
	targetPosition = GetStrategicTargetPosition();

	animator.Play("NIC_Fwd_Run", true);
}

//徘徊ステート更新処理
void EnemySlime::UpdateWanderState(float elapsedTime)
{
	//目標地点までXZ平面での距離判定
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float distSq = vx * vx + vz * vz;
	if (distSq < radius * radius)
	{
		//待機ステートへ偏移
		SetIdleState();
	}

	//目標地点へ移動
	MoveToTarget(elapsedTime, 1.0f, 1.0f);

	//  0.5秒ごとにターゲット索敵
	if (targetUpdateTimer <= 0.0f)
	{
		targetUpdateTimer = 0.5f;

		if (Character* target = SearchTarget())
		{
			SetAttackState(target);
		}
	}
}

//待機ステートへ偏移
void EnemySlime::SetIdleState()
{
	state = State::Idle;

	//タイマーをランダム設定
	stateTimer = MathUtils::RandomRenge(3.0f, 5.0f);

	animator.Play("NIC_Idle", true);
}

//待機ステート更新処理
void EnemySlime::UpdateIdleState(float elapsedTime)
{
	//タイマー処理
	stateTimer -= elapsedTime;
	if (stateTimer < 0.0f)
	{
		//徘徊ステートへ偏移
		SetWanderState();
	}

	// 0.5秒ごとにターゲット索敵
	if (targetUpdateTimer <= 0.0f)
	{
		targetUpdateTimer = 0.5f;

		if (Character* target = SearchTarget())
		{
			SetAttackState(target);
		}
	}
}

//ターゲット索敵
Character* EnemySlime::SearchTarget()
{
	float rangeSq = searchRange * searchRange;

	Character* bestTarget = nullptr;
	float closestDistSq = FLT_MAX;
	bool foundAlly = false; // 味方が見つかったかフラグ

	// AllySlime (味方) を検索
	// 味方が範囲内にいれば、プレイヤーとの距離に関係なく味方を優先する
	const auto& allies = AllySlime::GetAllAllies();
	for (Character* ally : allies)
	{
		if (!ally || ally->GetHealth() <= 0) continue;

		const DirectX::XMFLOAT3& pos = ally->GetPosition();
		float vx = pos.x - position.x;
		float vy = pos.y - position.y;
		float vz = pos.z - position.z;
		float distSq = (vx * vx + vy * vy + vz * vz);

		if (distSq < rangeSq)
		{
			// 味方を発見。味方同士の中で一番近いやつを選ぶ
			if (distSq < closestDistSq)
			{
				closestDistSq = distSq;
				bestTarget = ally;
			}
			foundAlly = true; // 味方発見フラグを立てる
		}
	}

	// 味方が見つかっているなら、プレイヤー探索はスキップしてその味方を返す
	if (foundAlly) return bestTarget;


	// 味方がいなければ Player を検索
	const auto& allPlayers = Player::GetAllPlayers();
	for (const auto* player : allPlayers)
	{
		if (!player || player->GetHealth() <= 0) continue;

		const DirectX::XMFLOAT3& playerPos = player->GetPosition();
		float vx = playerPos.x - position.x;
		float vy = playerPos.y - position.y;
		float vz = playerPos.z - position.z;
		float distSq = (vx * vx + vy * vy + vz * vz);

		if (distSq < rangeSq)
		{
			if (distSq < closestDistSq)
			{
				closestDistSq = distSq;
				bestTarget = const_cast<Player*>(player);
			}
		}
	}

	return bestTarget;
}

//攻撃ステートへ偏移
void EnemySlime::SetAttackState(Character* target)
{
	state = State::Attack;
	stateTimer = 0.0f;
	targetCharacter = target; // ★ targetPlayer から targetCharacter に変更

	animator.Play("NIC_Attack", true);
	isAttackFired = false;
}

//攻撃ステート更新処理
void EnemySlime::UpdateAttackState(float elapsedTime)
{
	// 攻撃中も0.5秒ごとにターゲットを再評価して、より優先度の高い相手がいれば切り替える
	if (targetUpdateTimer <= 0.0f)
	{
		targetUpdateTimer = 0.5f;

		Character* newTarget = SearchTarget();
		// 新しいターゲットが見つかったら切り替え (現在のターゲットより優先度が高い/近い可能性があるため)
		if (newTarget)
		{
			targetCharacter = newTarget;
		}
	}

	// ターゲットが有効かチェック (デストラクタで解除されるため)
	if (!targetCharacter)
	{
		SetIdleState();
		return;
	}

	//目標地点を "記憶したターゲットの" 位置に設定
	targetPosition = targetCharacter->GetPosition();

	//目標地点へ移動 (旋回のみ)
	MoveToTarget(elapsedTime, 0.0f, 1.0f);

	// 現在のアニメーション再生時間を取得
	float currentAnimTime = animator.GetCurrentSeconds();

	float fireTimingSeconds = 12.0f / 60.0f;

	// まだ撃っていなくて、かつ再生時間が発射タイミングを超えたら発射
	if (!isAttackFired && currentAnimTime >= fireTimingSeconds)
	{
		// --- 弾丸発射処理 (元のコードを使用) ---
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);

		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + height * 0.5f;
		pos.z = position.z;

		ProjectileStraite* projectile = new ProjectileStraite(&projectileManager, "Data/Model/Sword/RedSword.mdl");
		projectile->type = Type::EnemyAttack;
		projectile->Launch(dir, pos);
		// ----------------------------------

		// 「撃った」ことにする（連射防止）
		isAttackFired = true;
	}

	// アニメーションがループして時間が0に戻った場合、フラグをリセットして次弾に備える
	if (currentAnimTime < fireTimingSeconds && isAttackFired)
	{
		// 現在時間が発射時間より手前、かつ発射済みフラグが立っている＝ループした
		isAttackFired = false;
	}

	// ターゲットがまだ範囲内にいるか再チェック
	bool targetLost = true;
	const DirectX::XMFLOAT3& playerPositon = targetCharacter->GetPosition();
	float vx = playerPositon.x - position.x;
	float vy = playerPositon.y - position.y;
	float vz = playerPositon.z - position.z;
	float distSq = (vx * vx + vy * vy + vz * vz);

	if (distSq < searchRange * searchRange) // 範囲内
	{
		float distXZ = sqrtf(vx * vx + vz * vz);
		if (distXZ > 1e-6f) {
			float fvx = vx / distXZ;
			float fvz = vz / distXZ;
			float frontX = sinf(angle.y);
			float frontZ = cosf(angle.y);
			float dot = (frontX * fvx) + (frontZ * fvz);
			if (dot > 0.0f) // 前方
			{
				targetLost = false; // ★ 見失っていない
			}
		}
	}

	//プレイヤーを見失ったら
	if (targetLost)
	{
		//待機ステートへ偏移
		SetIdleState();
		targetCharacter = nullptr; // ★ ターゲットクリア
	}
}

//死亡した時に呼ばれる
void EnemySlime::OnDead()
{
	animator.Play("NIC_Death", false);

	CollisionManager::Instance().Remove(this);

	//自身を破棄
	Destroy();
}

void EnemySlime::OnCollision(GameObject* object)
{
	if (fabsf(mtd.x) > 1e-6f || fabsf(mtd.y) > 1e-6f || fabsf(mtd.z) > 1e-6f)
	{
		position.x += mtd.x; position.y += mtd.y; position.z += mtd.z;
	}
}

REGISTER_GAMEOBJECT(EnemySlime);

