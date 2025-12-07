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

// 視線判定用ヘルパー関数
static bool HasLineOfSight(const GridMap* map, const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end)
{
	if (!map) return true;

	auto s = map->WorldToCell(start.x, start.z);
	auto e = map->WorldToCell(end.x, end.z);

	int x0 = s.first, y0 = s.second;
	int x1 = e.first, y1 = e.second;

	int dx = abs(x1 - x0), dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (true)
	{
		if (map->IsBlocked(x0, y0)) return false; // 障害物あり
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 < dx) { err += dx; y0 += sy; }
	}
	return true;
}

// コンストラクタ
EnemySlime::EnemySlime(const char* modelPath)
{
	class_name = "EnemySlime";

	uniqueModel = ModelManager::Instance().CreateUniqueInstance(modelPath);
	model = uniqueModel.get();

	scale = { 0.005f, 0.005f, 0.005f };
	radius = 0.5f;
	height = 1.0f;
	type = Type::Enemy;
	maxHealth = 20;
	health = maxHealth;

	collider = std::make_unique<CylinderCollider>();
	collider->type = ColliderType::Cylinder;
	collider->owner = this;
	cylinder = static_cast<CylinderCollider*>(collider.get());
	cylinder->height = height;
	cylinder->radius = radius;

	CollisionManager::Instance().AddObject(this);

	UpdateTransform();
	SetWanderState();
}

EnemySlime::~EnemySlime() {}

// 更新処理
void EnemySlime::Update(float elapsedTime)
{
	targetUpdateTimer -= elapsedTime;
	cylinder->center = position;

	switch (state)
	{
	case State::Wander:	UpdateWanderState(elapsedTime); break;
	case State::Idle:	UpdateIdleState(elapsedTime);   break;
	case State::Attack:	UpdateAttackState(elapsedTime); break;
	}

	if (class_name != "EnemySlimeTurret")
	{
		ApplySeparationForce(elapsedTime);
	}

	UpdateVelocity(elapsedTime);
	UpdateInvincibleTimer(elapsedTime);
	UpdateTransform();

	projectileManager.Update(elapsedTime);
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

	if (gridMap && !currentPath.empty())
	{
		for (const auto& node : currentPath)
		{
			DirectX::XMFLOAT3 p = gridMap->GetWorldPosition(node.first, node.second);
			p.y = position.y + 0.5f;
			renderer->RenderSphere(rc, p, 0.1f, { 0, 1, 0, 1 });
		}
	}

	renderer->RenderCylinder(rc, territoryOrigin, territoryRange, 1.0f, { 0, 1, 0, 1 });
	renderer->RenderSphere(rc, targetPosition, 1.0f, { 1, 1, 0, 1 });
	renderer->RenderCylinder(rc, position, searchRange, 1.0f, { 1, 0, 0, 1 });
}

// ----------------------------------------------------------------------------
// AI & 戦略ロジック
// ----------------------------------------------------------------------------
bool EnemySlime::GetStrategicTarget(DirectX::XMFLOAT3& outPos)
{
	Core* core = Core::Instance();
	if (!core || core->GetHP() <= 0.0f) return false;

	const float kDefenseRange = 25.0f;
	float closestDistSq = FLT_MAX;
	DirectX::XMFLOAT3 defenseTarget = { 0,0,0 };
	bool isEmergency = false;

	const auto& players = Player::GetAllPlayers();
	for (const auto* p : players)
	{
		if (!p || p->GetHealth() <= 0) continue;
		float d2 = MathUtils::DistSqXZ(p->GetPosition(), core->position);
		if (d2 < kDefenseRange * kDefenseRange)
		{
			if (d2 < closestDistSq)
			{
				closestDistSq = d2;
				defenseTarget = p->GetPosition();
				isEmergency = true;
			}
		}
	}

	DirectX::XMFLOAT3 strategicPos;

	if (isEmergency)
	{
		strategicPos = defenseTarget;
	}
	else
	{
		int sectorCounts[8] = { 0 };
		DirectX::XMFLOAT3 corePos = core->position;

		auto CountUnit = [&](const DirectX::XMFLOAT3& pos) {
			float dx = pos.x - corePos.x;
			float dz = pos.z - corePos.z;
			float angle = atan2f(dx, dz);
			if (angle < 0.0f) angle += DirectX::XM_2PI;
			int sector = static_cast<int>(angle / (DirectX::XM_PI / 4.0f));
			sector = std::clamp(sector, 0, 7);
			sectorCounts[sector]++;
			};

		for (const auto* p : players) { if (p && p->GetHealth() > 0) CountUnit(p->GetPosition()); }
		const auto& allies = AllySlime::GetAllAllies();
		for (const auto* a : allies) { if (a && a->GetHealth() > 0) CountUnit(a->GetPosition()); }

		int maxSector = 0;
		int maxCount = -1;
		for (int i = 0; i < 8; ++i)
		{
			if (sectorCounts[i] > maxCount) {
				maxCount = sectorCounts[i];
				maxSector = i;
			}
		}

		if (maxCount <= 0) return false;

		float targetAngle = maxSector * (DirectX::XM_PI / 4.0f) + (DirectX::XM_PI / 8.0f);
		float distFromCore = 15.0f;

		strategicPos.x = corePos.x + sinf(targetAngle) * distFromCore;
		strategicPos.y = position.y;
		strategicPos.z = corePos.z + cosf(targetAngle) * distFromCore;
	}

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

	std::sort(candidates.begin(), candidates.end(),
		[](const Candidate& a, const Candidate& b) { return a.distSq < b.distSq; });

	bool isElite = false;
	int limit = isEmergency ? 8 : 4;

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
		float angleOffset = (float)(GetID() % 8) * (DirectX::XM_2PI / 8.0f);
		float radiusOffset = 3.0f + (float)(GetID() % 3);

		strategicPos.x += sinf(angleOffset) * radiusOffset;
		strategicPos.z += cosf(angleOffset) * radiusOffset;

		outPos = strategicPos;
		return true;
	}

	return false;
}

Character* EnemySlime::SearchTarget()
{
	float rangeSq = searchRange * searchRange;
	Character* bestTarget = nullptr;
	float closestDistSq = FLT_MAX;
	bool foundAlly = false;

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

void EnemySlime::ApplySeparationForce(float elapsedTime)
{
	DirectX::XMFLOAT3 separation = { 0, 0, 0 };
	int neighborCount = 0;
	float separationRadius = 1.5f;

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
			float strength = (separationRadius - dist) / dist;
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

void EnemySlime::SetWanderState()
{
	state = State::Wander;
	DirectX::XMFLOAT3 strategicPos;

	if (GetStrategicTarget(strategicPos))
	{
		targetPosition = strategicPos;
	}
	else
	{
		float theta = MathUtils::RandomRenge(-DirectX::XM_PI, DirectX::XM_PI);
		float range = MathUtils::RandomRenge(0.0f, territoryRange);
		targetPosition.x = territoryOrigin.x + sinf(theta) * range;
		targetPosition.y = territoryOrigin.y;
		targetPosition.z = territoryOrigin.z + cosf(theta) * range;
	}
}

void EnemySlime::UpdateWanderState(float elapsedTime)
{
	if (MathUtils::DistSqXZ(targetPosition, position) < 1.5f * 1.5f)
	{
		SetIdleState();
		return;
	}

	if (!MoveToTarget(elapsedTime, 1.0f, 1.0f))
	{
		SetIdleState();
		return;
	}

	if (targetUpdateTimer <= 0.0f)
	{
		targetUpdateTimer = 0.5f;

		if (Character* target = SearchTarget())
		{
			SetAttackState(target);
			return;
		}

		DirectX::XMFLOAT3 newStrategicPos;
		if (GetStrategicTarget(newStrategicPos))
		{
			float distSq = MathUtils::DistSqXZ(targetPosition, newStrategicPos);
			if (distSq > 25.0f)
			{
				targetPosition = newStrategicPos;
			}
		}
	}
}

void EnemySlime::SetIdleState()
{
	state = State::Idle;
	stateTimer = MathUtils::RandomRenge(3.0f, 5.0f);
}

void EnemySlime::UpdateIdleState(float elapsedTime)
{
	stateTimer -= elapsedTime;
	if (stateTimer < 0.0f)
	{
		SetWanderState();
		return;
	}

	if (targetUpdateTimer <= 0.0f)
	{
		targetUpdateTimer = 0.5f;
		if (Character* target = SearchTarget())
		{
			SetAttackState(target);
		}
	}
}

void EnemySlime::SetAttackState(Character* target)
{
	state = State::Attack;
	stateTimer = 0.0f;
	targetCharacter = target;
	isAttackFired = false;
	motionTimer = 0.0f;
}

void EnemySlime::UpdateAttackState(float elapsedTime)
{
	// ターゲット再評価
	if (targetUpdateTimer <= 0.0f)
	{
		targetUpdateTimer = 0.5f;
		Character* newTarget = SearchTarget();
		if (newTarget) targetCharacter = newTarget;
	}

	if (!targetCharacter || targetCharacter->GetHealth() <= 0)
	{
		SetIdleState();
		return;
	}

	targetPosition = targetCharacter->GetPosition();

	// 敵へのベクトル
	float dx = targetPosition.x - position.x;
	float dz = targetPosition.z - position.z;
	float distSq = dx * dx + dz * dz;

	// ★修正: 攻撃中は素早くターゲットを向く
	if (distSq > 0.0001f)
	{
		Turn(elapsedTime, dx, dz, turnSpeed * 3.0f);
	}

	// ★修正: 正面判定
	float fx = sinf(angle.y);
	float fz = cosf(angle.y);
	float dist = sqrtf(distSq);
	float tx = (dist > 0.0f) ? dx / dist : 0.0f;
	float tz = (dist > 0.0f) ? dz / dist : 1.0f;
	float dot = fx * tx + fz * tz;

	// 発射タイミング (例: 0.2秒時点)
	const float attackCycleDuration = 1.0f;
	const float fireTiming = 0.2f;

	// まだ向いていない場合 (角度差18度以上)、発射タイミング前なら待機
	if (dot < 0.95f && motionTimer < fireTiming)
	{
		// returnしてタイマーを進めない
		return;
	}

	motionTimer += elapsedTime;

	if (!isAttackFired && motionTimer >= fireTiming)
	{
		DirectX::XMFLOAT3 dir = { sinf(angle.y), 0.0f, cosf(angle.y) };
		DirectX::XMFLOAT3 pos = { position.x, position.y + height * 0.5f, position.z };

		ProjectileStraite* projectile = new ProjectileStraite(&projectileManager, "Data/Model/Slime/Bullet.mdl");
		projectile->type = Type::EnemyAttack;
		projectile->Launch(dir, pos);

		isAttackFired = true;
	}

	if (motionTimer >= attackCycleDuration)
	{
		motionTimer -= attackCycleDuration;
		isAttackFired = false;
	}

	if (distSq > searchRange * searchRange)
	{
		SetIdleState();
		targetCharacter = nullptr;
	}
}

// MoveToTarget等は省略 (既存のまま)
bool EnemySlime::MoveToTarget(float elapsedTime, float moveSpeedRate, float turnSpeedRate)
{
	float vx = 0.0f, vz = 0.0f;
	bool shouldMove = false;

	if (gridMap && moveSpeedRate > 0.0f)
	{
		pathRecalcTimer -= elapsedTime;
		float distToPrev = MathUtils::DistSqXZ(targetPosition, prevTargetPos);
		bool targetMoved = (distToPrev > 1.0f);

		if (currentPath.empty() || targetMoved)
		{
			if (pathRecalcTimer <= 0.0f || targetMoved)
			{
				pathRecalcTimer = 0.5f;
				prevTargetPos = targetPosition;

				auto start = gridMap->WorldToCell(position.x, position.z);
				auto goal = gridMap->WorldToCell(targetPosition.x, targetPosition.z);

				auto rawPath = aStar.ReplanPath(
					start.first, start.second,
					goal.first, goal.second,
					*gridMap,
					start.first, start.second
				);
				currentPath = rawPath;

				if (currentPath.empty())
				{
					float d2 = MathUtils::DistSqXZ(position, targetPosition);
					if (d2 > 4.0f)
					{
						if (!HasLineOfSight(gridMap, position, targetPosition)) return false;
					}
				}

				pathIndex = 0;
				if (!currentPath.empty())
				{
					int closestIndex = 0;
					float minDistSq = FLT_MAX;
					for (int i = 0; i < static_cast<int>(currentPath.size()); ++i)
					{
						DirectX::XMFLOAT3 nodePos = gridMap->GetWorldPosition(currentPath[i].first, currentPath[i].second);
						float d = MathUtils::DistSqXZ(position, nodePos);
						if (d < minDistSq)
						{
							if (HasLineOfSight(gridMap, position, nodePos))
							{
								minDistSq = d;
								closestIndex = i;
							}
						}
					}
					pathIndex = closestIndex;

					if (pathIndex + 1 < currentPath.size())
					{
						DirectX::XMFLOAT3 nodePos = gridMap->GetWorldPosition(currentPath[pathIndex].first, currentPath[pathIndex].second);
						DirectX::XMFLOAT3 nextNodePos = gridMap->GetWorldPosition(currentPath[pathIndex + 1].first, currentPath[pathIndex + 1].second);
						float pathVecX = nextNodePos.x - nodePos.x;
						float pathVecZ = nextNodePos.z - nodePos.z;
						float myVecX = position.x - nodePos.x;
						float myVecZ = position.z - nodePos.z;
						float dot = pathVecX * myVecX + pathVecZ * myVecZ;
						if (dot > 0.0f) pathIndex++;
					}
				}
			}
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
			float dx = targetPosition.x - position.x;
			float dz = targetPosition.z - position.z;
			float distSq = dx * dx + dz * dz;

			if (distSq > 0.5f * 0.5f)
			{
				if (HasLineOfSight(gridMap, position, targetPosition))
				{
					float dist = sqrtf(distSq);
					if (dist > 0.001f) { vx = dx / dist; vz = dz / dist; shouldMove = true; }
				}
				else return false;
			}
		}
	}
	else if (moveSpeedRate > 0.0f)
	{
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

	return true;
}

void EnemySlime::SetTerritory(const DirectX::XMFLOAT3& origin, float range)
{
	territoryOrigin = origin;
	territoryRange = range;

	if (state == State::Wander)
	{
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
	CollisionManager::Instance().Remove(this);
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