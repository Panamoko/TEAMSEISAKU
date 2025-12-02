#include "MathUtils.h"
#include "EnemySlime.h"
#include "ProjectileStraite.h"
#include "Player.h"
#include "AllySlime.h"
#include "Core.h"
#include "Collision.h"
#include "EnemyManager.h"
#include <cfloat>
#include <algorithm> // clamp, sort

// コンストラクタ
EnemySlime::EnemySlime(const char* modelPath)
{
	class_name = "EnemySlime";

	// モデルロード (各個体でアニメーションさせるためUniqueInstance)
	uniqueModel = ModelManager::Instance().CreateUniqueInstance(modelPath);
	model = uniqueModel.get();

	// パラメータ設定
	scale = { 0.01f, 0.01f, 0.01f };
	radius = 0.5f;
	height = 1.0f;
	type = Type::Enemy;

	// コライダー設定
	collider = std::make_unique<CylinderCollider>();
	collider->type = ColliderType::Cylinder;
	collider->owner = this;
	cylinder = static_cast<CylinderCollider*>(collider.get());
	cylinder->height = height;
	cylinder->radius = radius;

	CollisionManager::Instance().AddObject(this);

	// アニメーション初期化
	if (model) {
		animator.SetModel(model);
		animator.Play("NIC_Idle", true);
		model->UpdateTransform();
	}
	UpdateTransform();

	// 初期状態へ
	SetWanderState();
}

EnemySlime::~EnemySlime() {}

// 更新処理
void EnemySlime::Update(float elapsedTime)
{
	targetUpdateTimer -= elapsedTime;
	cylinder->center = position;

	// ステート更新
	switch (state)
	{
	case State::Wander:	UpdateWanderState(elapsedTime); break;
	case State::Idle:	UpdateIdleState(elapsedTime);   break;
	case State::Attack:	UpdateAttackState(elapsedTime); break;
	}

	animator.Update(elapsedTime);

	// 固定砲台以外は「分離行動（重なり回避）」を行う
	if (class_name != "EnemySlimeTurret")
	{
		ApplySeparationForce(elapsedTime);
	}

	// 物理・移動更新
	UpdateVelocity(elapsedTime);
	UpdateInvincibleTimer(elapsedTime);
	UpdateTransform(); // 行列更新

	// 弾の更新
	projectileManager.Update(elapsedTime);

	// モデル更新
	if (model) model->UpdateTransform();
}

// 描画処理
void EnemySlime::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, model, ShaderId::Lambert, GetDamageColor());
	projectileManager.Render(rc, renderer);
}

// デバッグ描画
void EnemySlime::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	Enemy::RenderDebugPrimitive(rc, renderer);

	// 経路の可視化
	if (gridMap && !currentPath.empty())
	{
		for (const auto& node : currentPath)
		{
			DirectX::XMFLOAT3 p = gridMap->GetWorldPosition(node.first, node.second);
			p.y = position.y + 0.5f;
			renderer->RenderSphere(rc, p, 0.1f, { 0, 1, 0, 1 });
		}
	}

	// 各種範囲の可視化
	renderer->RenderCylinder(rc, territoryOrigin, territoryRange, 1.0f, { 0, 1, 0, 1 }); // 縄張り
	renderer->RenderSphere(rc, targetPosition, 1.0f, { 1, 1, 0, 1 });                    // 目的地
	renderer->RenderCylinder(rc, position, searchRange, 1.0f, { 1, 0, 0, 1 });           // 索敵範囲
}

// ----------------------------------------------------------------------------
// AI & 戦略ロジック
// ----------------------------------------------------------------------------

// 戦略的なターゲット座標の計算
bool EnemySlime::GetStrategicTarget(DirectX::XMFLOAT3& outPos)
{
	Core* core = Core::Instance();
	if (!core || core->GetHP() <= 0.0f) return false;

	// 1. コア周辺の8方向エリアで、プレイヤー・味方の数をカウント
	int sectorCounts[8] = { 0 };
	DirectX::XMFLOAT3 corePos = core->position;

	auto CountUnit = [&](const DirectX::XMFLOAT3& pos) {
		float dx = pos.x - corePos.x;
		float dz = pos.z - corePos.z;
		float angle = atan2f(dx, dz); // (x, z) 0時方向=0, 時計回り
		if (angle < 0.0f) angle += DirectX::XM_2PI;
		int sector = static_cast<int>(angle / (DirectX::XM_PI / 4.0f));
		sector = std::clamp(sector, 0, 7);
		sectorCounts[sector]++;
		};

	const auto& players = Player::GetAllPlayers();
	for (const auto* p : players) { if (p && p->GetHealth() > 0) CountUnit(p->GetPosition()); }
	const auto& allies = AllySlime::GetAllAllies();
	for (const auto* a : allies) { if (a && a->GetHealth() > 0) CountUnit(a->GetPosition()); }

	// 最も敵が多いセクターを探す
	int maxSector = 0;
	int maxCount = -1;
	for (int i = 0; i < 8; ++i)
	{
		if (sectorCounts[i] > maxCount) {
			maxCount = sectorCounts[i];
			maxSector = i;
		}
	}

	if (maxCount <= 0) return false; // 敵がいなければ戦略移動不要

	// 2. ターゲット地点（激戦区の中心）を決定
	float targetAngle = maxSector * (DirectX::XM_PI / 4.0f) + (DirectX::XM_PI / 8.0f);
	float distFromCore = 15.0f; // コアから少し離れた位置で迎撃

	DirectX::XMFLOAT3 strategicPos;
	strategicPos.x = corePos.x + sinf(targetAngle) * distFromCore;
	strategicPos.y = position.y;
	strategicPos.z = corePos.z + cosf(targetAngle) * distFromCore;

	// 3. 自分はこの地点に近い「選抜メンバー（上位4体）」か？
	struct Candidate { Enemy* enemy; float distSq; };
	std::vector<Candidate> candidates;

	EnemyManager& em = EnemyManager::Instance();
	for (int i = 0; i < em.GetEnemyCount(); ++i)
	{
		auto enemy = em.GetEnemy(i);
		if (!enemy || enemy->IsDestroyRequested() || enemy->class_name != this->class_name) continue;

		float dx = enemy->GetPosition().x - strategicPos.x;
		float dz = enemy->GetPosition().z - strategicPos.z;
		candidates.push_back({ enemy.get(), dx * dx + dz * dz });
	}

	// 距離順にソート
	std::sort(candidates.begin(), candidates.end(),
		[](const Candidate& a, const Candidate& b) { return a.distSq < b.distSq; });

	// 上位に入っているかチェック
	bool isElite = false;
	int limit = 4;
	for (int i = 0; i < (int)candidates.size(); ++i)
	{
		if (i >= limit) break;
		if (candidates[i].enemy == this) {
			isElite = true;
			break;
		}
	}

	if (isElite)
	{
		// 密集回避のため少し散らす
		strategicPos.x += MathUtils::RandomRenge(-2.0f, 2.0f);
		strategicPos.z += MathUtils::RandomRenge(-2.0f, 2.0f);
		outPos = strategicPos;
		return true;
	}

	return false;
}

// ターゲット索敵
Character* EnemySlime::SearchTarget()
{
	float rangeSq = searchRange * searchRange;
	Character* bestTarget = nullptr;
	float closestDistSq = FLT_MAX;
	bool foundAlly = false;

	// 1. AllySlime (味方) を優先検索
	const auto& allies = AllySlime::GetAllAllies();
	for (Character* ally : allies)
	{
		if (!ally || ally->GetHealth() <= 0) continue;
		float d2 = MathUtils::DistSqXZ(ally->GetPosition(), position);
		if (d2 < rangeSq)
		{
			if (d2 < closestDistSq) {
				closestDistSq = d2;
				bestTarget = ally;
			}
			foundAlly = true;
		}
	}
	if (foundAlly) return bestTarget;

	// 2. Player を検索
	const auto& players = Player::GetAllPlayers();
	for (const auto* player : players)
	{
		if (!player || player->GetHealth() <= 0) continue;
		float d2 = MathUtils::DistSqXZ(player->GetPosition(), position);
		if (d2 < rangeSq && d2 < closestDistSq)
		{
			closestDistSq = d2;
			bestTarget = const_cast<Player*>(player);
		}
	}
	return bestTarget;
}

// 分離行動（重なり回避）
void EnemySlime::ApplySeparationForce(float elapsedTime)
{
	DirectX::XMFLOAT3 separation = { 0, 0, 0 };
	int neighborCount = 0;
	float separationRadius = 1.5f; // 反発しあう距離

	EnemyManager& em = EnemyManager::Instance();
	for (int i = 0; i < em.GetEnemyCount(); ++i)
	{
		auto other = em.GetEnemy(i);
		if (!other || other.get() == this || other->IsDestroyRequested()) continue;

		float dx = position.x - other->GetPosition().x;
		float dz = position.z - other->GetPosition().z;
		float distSq = dx * dx + dz * dz;

		if (distSq < separationRadius * separationRadius && distSq > 0.0001f)
		{
			float dist = sqrtf(distSq);
			float strength = (separationRadius - dist) / dist; // 近いほど強い
			separation.x += dx * strength;
			separation.z += dz * strength;
			neighborCount++;
		}
	}

	if (neighborCount > 0)
	{
		separation.x /= neighborCount;
		separation.z /= neighborCount;
		float forceStrength = 5.0f;
		velocity.x += separation.x * forceStrength * elapsedTime;
		velocity.z += separation.z * forceStrength * elapsedTime;
	}
}

// ----------------------------------------------------------------------------
// ステートマシン: 徘徊
// ----------------------------------------------------------------------------
void EnemySlime::SetWanderState()
{
	state = State::Wander;
	DirectX::XMFLOAT3 strategicPos;

	// 戦略目標があるか確認
	if (GetStrategicTarget(strategicPos))
	{
		targetPosition = strategicPos;
	}
	else
	{
		// なければランダム移動
		float theta = MathUtils::RandomRenge(-DirectX::XM_PI, DirectX::XM_PI);
		float range = MathUtils::RandomRenge(0.0f, territoryRange);
		targetPosition.x = territoryOrigin.x + sinf(theta) * range;
		targetPosition.y = territoryOrigin.y;
		targetPosition.z = territoryOrigin.z + cosf(theta) * range;
	}
	animator.Play("NIC_Fwd_Run", true);
}

void EnemySlime::UpdateWanderState(float elapsedTime)
{
	// 到着判定
	if (MathUtils::DistSqXZ(targetPosition, position) < radius * radius)
	{
		SetIdleState();
		return;
	}

	MoveToTarget(elapsedTime, 1.0f, 1.0f);

	// 0.5秒ごとの思考
	if (targetUpdateTimer <= 0.0f)
	{
		targetUpdateTimer = 0.5f;

		// 1. 敵発見なら攻撃へ
		if (Character* target = SearchTarget())
		{
			SetAttackState(target);
			return;
		}

		// 2. 戦況が変わったら進路変更
		DirectX::XMFLOAT3 newStrategicPos;
		if (GetStrategicTarget(newStrategicPos))
		{
			targetPosition = newStrategicPos;
		}
	}
}

// ----------------------------------------------------------------------------
// ステートマシン: 待機
// ----------------------------------------------------------------------------
void EnemySlime::SetIdleState()
{
	state = State::Idle;
	stateTimer = MathUtils::RandomRenge(3.0f, 5.0f);
	animator.Play("NIC_Idle", true);
}

void EnemySlime::UpdateIdleState(float elapsedTime)
{
	// 時間経過で再び徘徊へ
	stateTimer -= elapsedTime;
	if (stateTimer < 0.0f)
	{
		SetWanderState();
		return;
	}

	// 待機中も索敵
	if (targetUpdateTimer <= 0.0f)
	{
		targetUpdateTimer = 0.5f;
		if (Character* target = SearchTarget())
		{
			SetAttackState(target);
		}
	}
}

// ----------------------------------------------------------------------------
// ステートマシン: 攻撃
// ----------------------------------------------------------------------------
void EnemySlime::SetAttackState(Character* target)
{
	state = State::Attack;
	stateTimer = 0.0f;
	targetCharacter = target;
	isAttackFired = false;
	animator.Play("NIC_Attack", true);
}

void EnemySlime::UpdateAttackState(float elapsedTime)
{
	// ターゲットの再評価 (0.5秒ごと)
	if (targetUpdateTimer <= 0.0f)
	{
		targetUpdateTimer = 0.5f;
		Character* newTarget = SearchTarget();
		if (newTarget) targetCharacter = newTarget;
	}

	// ターゲット無効チェック
	if (!targetCharacter || targetCharacter->GetHealth() <= 0)
	{
		SetIdleState();
		return;
	}

	targetPosition = targetCharacter->GetPosition();
	MoveToTarget(elapsedTime, 0.0f, 1.0f); // 旋回のみ

	// アニメーションに合わせて発射
	float currentAnimTime = animator.GetCurrentSeconds();
	float fireTimingSeconds = 12.0f / 60.0f;

	if (!isAttackFired && currentAnimTime >= fireTimingSeconds)
	{
		DirectX::XMFLOAT3 dir = { sinf(angle.y), 0.0f, cosf(angle.y) };
		DirectX::XMFLOAT3 pos = { position.x, position.y + height * 0.5f, position.z };

		ProjectileStraite* projectile = new ProjectileStraite(&projectileManager, "Data/Model/Sword/RedSword.mdl");
		projectile->type = Type::EnemyAttack;
		projectile->Launch(dir, pos);

		isAttackFired = true;
	}

	// アニメーションループ判定
	if (currentAnimTime < fireTimingSeconds && isAttackFired)
	{
		isAttackFired = false;
	}

	// 距離チェック
	if (MathUtils::DistSqXZ(targetCharacter->GetPosition(), position) > searchRange * searchRange)
	{
		SetIdleState();
		targetCharacter = nullptr;
	}
}

// ----------------------------------------------------------------------------
// その他
// ----------------------------------------------------------------------------

// A*を使った移動処理
void EnemySlime::MoveToTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate)
{
	float vx = 0.0f, vz = 0.0f;
	bool shouldMove = false;

	if (gridMap && moveSpeedRate > 0.0f)
	{
		// 定期的に経路更新
		pathRecalcTimer -= elapsedTime;
		if (pathRecalcTimer <= 0.0f)
		{
			pathRecalcTimer = 0.5f;
			auto start = gridMap->WorldToCell(position.x, position.z);
			auto goal = gridMap->WorldToCell(targetPosition.x, targetPosition.z);
			auto rawPath = aStar.FindPath(start.first, start.second, goal.first, goal.second, *gridMap);
			currentPath = rawPath.empty() ? rawPath : aStar.SmoothPath(rawPath, *gridMap);
			pathIndex = 0;
		}

		if (!currentPath.empty() && pathIndex < currentPath.size())
		{
			auto nextNode = currentPath[pathIndex];
			DirectX::XMFLOAT3 nextPos = gridMap->GetWorldPosition(nextNode.first, nextNode.second);
			float dx = nextPos.x - position.x;
			float dz = nextPos.z - position.z;
			float dist = sqrtf(dx * dx + dz * dz);

			if (dist < 0.5f) {
				pathIndex++;
			}
			else {
				vx = dx / dist; vz = dz / dist;
				shouldMove = true;
			}
		}
		else
		{
			// 経路なし or 到着済みなら直進トライ
			float dx = targetPosition.x - position.x;
			float dz = targetPosition.z - position.z;
			float dist = sqrtf(dx * dx + dz * dz);
			if (dist > 0.001f) { vx = dx / dist; vz = dz / dist; shouldMove = true; }
		}
	}
	else if (moveSpeedRate > 0.0f)
	{
		// マップなしの場合は直進
		float dx = targetPosition.x - position.x;
		float dz = targetPosition.z - position.z;
		float dist = sqrtf(dx * dx + dz * dz);
		if (dist > 0.001f) { vx = dx / dist; vz = dz / dist; shouldMove = true; }
	}

	if (shouldMove)
	{
		Move(elapsedTime, vx, vz, moveSpeed * moveSpeedRate);
		Turn(elapsedTime, vx, vz, turnSpeed * turnSpeedRate);
	}
}

void EnemySlime::SetTerritory(const DirectX::XMFLOAT3& origin, float range)
{
	territoryOrigin = origin;
	territoryRange = range;

	if (state == State::Wander)
	{
		// 戦略的ターゲット優先、なければランダム
		DirectX::XMFLOAT3 strategicPos;
		if (GetStrategicTarget(strategicPos))
		{
			targetPosition = strategicPos;
		}
		else
		{
			float theta = MathUtils::RandomRenge(-DirectX::XM_PI, DirectX::XM_PI);
			float r = MathUtils::RandomRenge(0.0f, territoryRange);
			targetPosition.x = territoryOrigin.x + sinf(theta) * r;
			targetPosition.y = territoryOrigin.y;
			targetPosition.z = territoryOrigin.z + cosf(theta) * r;
		}
	}
}

void EnemySlime::OnDead()
{
	animator.Play("NIC_Death", false);
	CollisionManager::Instance().Remove(this);
	Destroy();
}

void EnemySlime::OnCollision(GameObject* object)
{
	// MTD補正（押し出し）
	if (fabsf(mtd.x) > 1e-6f || fabsf(mtd.y) > 1e-6f || fabsf(mtd.z) > 1e-6f)
	{
		position.x += mtd.x; position.y += mtd.y; position.z += mtd.z;
	}
}

REGISTER_GAMEOBJECT(EnemySlime);