#include "Collision.h"
#include "Player.h"
#include "EnemyManager.h"
#include "Enemy.h"
#include "Camera.h"
#include "System/Input.h"
#include "Picking_Ray.h"
#include <imgui.h>
#include <cfloat>
#include <cmath>
#include "System/Graphics.h"
#include <DirectXMath.h>
#include <d3d11.h>
#include <algorithm>
#include "Core.h" 
#include "GridMap.h"
#include "MathUtils.h"

using namespace DirectX;


Player* Player::sActive = nullptr;
std::vector<Player*> Player::sAllPlayers;
Player& Player::Instance() { return *sActive; }
void Player::SetActive(Player* p) { sActive = p; }
Player* Player::GetActivePtr() { return sActive; }

// ========= 全プレイヤーリスト管理 =========
void Player::RegisterPlayer(Player* player)
{
	if (player) {
		// 重複を避ける（念のため）
		auto it = std::find(sAllPlayers.begin(), sAllPlayers.end(), player);
		if (it == sAllPlayers.end()) {
			sAllPlayers.push_back(player);
		}
	}
}

void Player::UnregisterPlayer(Player* player)
{
	// Erase-Removeイディオム
	sAllPlayers.erase(
		std::remove(sAllPlayers.begin(), sAllPlayers.end(), player),
		sAllPlayers.end()
	);
}

const std::vector<Player*>& Player::GetAllPlayers()
{
	return sAllPlayers;
}

//初期化
void Player::Initialize()
{
	//親スライムは攻撃しない
	autoAttackEnabled = false;

	//直接new Modelせず、Manager経由でインスタンス生成（高速化）
		// model = new Model("Data/Model/Slime/Player_Slime.mdl");
	model = ModelManager::Instance().CreateNewInstance("Data/Model/Slime/Player_Slime.mdl");
	// モデルが大きいのでスケーリング
	scale.x = scale.y = scale.z = 0.005f;

	// 3. アニメーターには、この「自分専用のコピー」をセットする
	animator.SetModel(model);
	//頭のボーン(joint4)と王冠(pTorus1)の番号を探して記憶する
	const auto& nodes = model->GetNodes();
	for (int i = 0; i < nodes.size(); ++i)
	{
		// ノード名をstd::stringとして取得
		std::string nodeName = nodes[i].name;

		if (nodeName.find("joint4") != std::string::npos)
		{
			headBoneIndex = i;
		}
		else if (nodeName.find("pTorus1") != std::string::npos)
		{
			crownNodeIndex = i;
		}
	}
	RegisterPlayer(this);

	type = Type::Player;
	collider = std::make_unique<CylinderCollider>();
	collider->type = ColliderType::Cylinder;
	collider->owner = this;
	cylinder = static_cast<CylinderCollider*>(collider.get());

	cylinder->height = height;
	cylinder->radius = radius;

	CollisionManager::Instance().AddObject(this);

	animator.SetModel(model /* or model.get() */);
	animator.SetBlendSeconds(0.2f);
	animator.Play("Take 001", true);

	pathRecalcTimer = MathUtils::RandomRenge(0.0f, 0.5f);
}

//終了化
void Player::Finalize()
{
		model = nullptr;
}

void Player::Update(float elapsedTime)
{
	animator.Update(elapsedTime);
	cylinder->center = position;

	const bool isActive = (this == GetActivePtr());
	if (isActive) {
		// 入力は“選択された個体のみ”
		InputToggleAttackPriority();
		InputMove(elapsedTime);
		InputJump();
		InputProjectile();
		//AutoAttackUpdate(elapsedTime);   // ← 手動操作時は自動攻撃しない
	}
	else {
		// 非アクティブ（自動）時
		// AutoAttackUpdate(elapsedTime); // ★親スライムの攻撃はOFFにするためコメントアウト

		// ★追加: コアへの自動移動
		UpdateMoveToCore(elapsedTime);
	}

	// ★注: UpdateAutoMoveToEnemy(elapsedTime) は使わずに UpdateMoveToCore を使う

   //速力更新処理
	UpdateVelocity(elapsedTime);

	//弾丸更新処理
	projectileManager.Update(elapsedTime);

	//プレイヤーと敵との衝突処理
	CollisionPlayerVsEnemies();

	//プレイヤーと柵との衝突処理
	CollisionPlayerVsFences();

	// オブジェクト行列を更新
	UpdateTransform();

	// モデル行列更新
	model->UpdateTransform();

	// 王冠の行列を、頭のボーンの行列で上書きして強制的に追従させる
	if (headBoneIndex != -1 && crownNodeIndex != -1)
	{
		Model::Node* nodesPtr = const_cast<Model::Node*>(model->GetNodes().data());
		auto& headNode = nodesPtr[headBoneIndex];
		auto& crownNode = nodesPtr[crownNodeIndex];
		crownNode.globalTransform = headNode.globalTransform;
	}
}

// ★追加: コアへ向かう処理
void Player::UpdateMoveToCore(float elapsedTime)
{
	Core* core = Core::Instance();
	// コアが存在しない、またはHPがない場合は動かない
	if (!core || core->GetHP() <= 0.0f) {
		Move(elapsedTime, 0, 0, 0);
		return;
	}

	if (!gridMap) return;

	// --- 修正: 定期的に経路を再計算 (onceフラグは使わない) ---
	pathRecalcTimer -= elapsedTime;

	// 経路がない、またはタイマーが切れたら再計算
	if (currentPath.empty() || pathRecalcTimer <= 0.0f)
	{
		pathRecalcTimer = 0.5f; // 0.5秒ごとに更新

		auto start = gridMap->WorldToCell(position.x, position.z);
		auto goal = gridMap->WorldToCell(core->position.x, core->position.z);

		// A*で経路探索
		currentPath = aStar.FindPath(start.first, start.second, goal.first, goal.second, *gridMap);

		if (currentPath.empty()) {
			// これが出ているなら「スタート地点が壁の中」か「ゴールまで道がない」
			printf("Path Not Found! Start(%d,%d) -> Goal(%d,%d)\n", start.first, start.second, goal.first, goal.second);
		}
		else {
			// これが出ているのに動かないなら、移動処理(Move)か到達判定がおかしい
			printf("Path Found! Size: %d\n", (int)currentPath.size());
		}

		// 経路が見つかった場合、インデックスをリセット
		// [0]は現在地なので、[1]（次の地点）を目指すのが基本
		if (!currentPath.empty())
		{
			pathIndex = (currentPath.size() > 1) ? 1 : 0;
		}
	}

	DirectX::XMFLOAT3 targetPos = core->position;
	bool hasPath = !currentPath.empty();

	// 経路がある場合は次のノードを目指す
	if (hasPath && pathIndex < currentPath.size())
	{
		auto [cx, cz] = currentPath[pathIndex];
		targetPos = gridMap->GetWorldPosition(cx, cz);

		// --- 修正: XZ平面での距離チェック (高さyは無視) ---
		float dx = targetPos.x - position.x;
		float dz = targetPos.z - position.z;
		float distSq = dx * dx + dz * dz;

		// 到達判定（半径0.5m以内なら次へ）
		if (distSq < 0.5f * 0.5f)
		{
			pathIndex++;
			// まだ続きがあるならターゲット位置を即時更新
			if (pathIndex < currentPath.size()) {
				auto [nextCx, nextCz] = currentPath[pathIndex];
				targetPos = gridMap->GetWorldPosition(nextCx, nextCz);
			}
		}
	}
	else if (!hasPath)
	{
		// 経路が見つからない場合は停止（無理に進むと壁に埋まるため）
		Move(elapsedTime, 0, 0, 0);
		return;
	}

	// 移動処理
	float vx = targetPos.x - position.x;
	float vz = targetPos.z - position.z;
	float dist = sqrtf(vx * vx + vz * vz);

	if (dist > 0.001f)
	{
		vx /= dist;
		vz /= dist;

		// 経路の終端（コア付近）に来たら手前で止まる制御
		if (pathIndex >= currentPath.size())
		{
			float distToCoreSq =
				(core->position.x - position.x) * (core->position.x - position.x) +
				(core->position.z - position.z) * (core->position.z - position.z);

			// コアの手前 3.0f で停止
			if (distToCoreSq < 3.0f * 3.0f)
			{
				Move(elapsedTime, 0, 0, 0);
				return;
			}
		}

		Move(elapsedTime, vx, vz, moveSpeed * autoMoveSpeedRate);
		Turn(elapsedTime, vx, vz, turnSpeed * autoMoveTurnRate);
	}
	else
	{
		Move(elapsedTime, 0, 0, 0);
	}
}

void Player::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, model, ShaderId::Lambert);

	//弾丸描画処理
	projectileManager.Render(rc, renderer);
}

// デバッグ用GUI描画
void Player::DrawDebugGUI()
{
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	ImGui::SetNextWindowPos(ImVec2(pos.x + 10, pos.y + 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Player", nullptr, ImGuiWindowFlags_None))
	{
		// トランスフォーム
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 位置
			ImGui::InputFloat3("Position", &position.x);

			// 回転
			DirectX::XMFLOAT3 a;
			a.x = DirectX::XMConvertToDegrees(angle.x);
			a.y = DirectX::XMConvertToDegrees(angle.y);
			a.z = DirectX::XMConvertToDegrees(angle.z);
			ImGui::InputFloat3("Angle", &a.x);

			angle.x = DirectX::XMConvertToRadians(a.x);
			angle.y = DirectX::XMConvertToRadians(a.y);
			angle.z = DirectX::XMConvertToRadians(a.z);

			// スケール
			ImGui::InputFloat3("Scale", &scale.x);
		}
		// 自動攻撃のデバッグ設定
		if (ImGui::CollapsingHeader("Auto Attack", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Enabled", &autoAttackEnabled);
			ImGui::DragFloat("Range", &autoAttackRange, 0.1f, 0.0f, 100.0f);
			ImGui::DragFloat("Interval (sec)", &autoAttackInterval, 0.01f, 0.1f, 10.0f);
			// 現在のタイマー表示
			ImGui::Text("Timer: %.2f", autoAttackTimer);
			// ★ 優先度切り替えボタン
			const char* modeLabel = (attackPriority == AttackPriority::CoreFirst)
				? "Core → Enemy"
				: "Enemy → Core";
			if (ImGui::Button(modeLabel, ImVec2(150, 0)))
			{
				attackPriority = (attackPriority == AttackPriority::CoreFirst)
					? AttackPriority::EnemyFirst
					: AttackPriority::CoreFirst;
			}
		}
	}

	ImGui::End();
}



DirectX::XMFLOAT3 Player::GetMoveVec() const
{
	// 入力情報を取得
	GamePad& gamePad = Input::Instance().GetGamePad();
	float ax = gamePad.GetAxisLX(); // スティックのX軸入力
	float ay = gamePad.GetAxisLY(); // スティックのY軸入力

	//  カメラ方向ではなく、ワールド軸（X軸:右、Z軸:前）を基準に移動ベクトルを計算する
	DirectX::XMFLOAT3 vec;

	// スティックX軸 (ax) をワールドX軸（右方向）に、
	// スティックY軸 (ay) をワールドZ軸（前方向）に割り当てる
	vec.x = ax; // ax: -1.0(左)  +1.0(右) -> ワールドX軸
	vec.z = ay; // ay: -1.0(下)  +1.0(上) -> ワールドZ軸
	vec.y = 0.0f; // Y軸方向には移動しない

	// ベクトルの正規化（斜め移動の速度が速くならないように）
	float lengthSq = vec.x * vec.x + vec.z * vec.z;
	if (lengthSq > 1.0f)
	{
		float length = sqrtf(lengthSq);
		vec.x /= length;
		vec.z /= length;
	}

	return vec;
}

// 移動入力処理
void Player::InputMove(float elapsedTime)
{
	// 進行ベクトル取得
	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	// 移動処理
	Move(elapsedTime, moveVec.x, moveVec.z, moveSpeed);

	// 旋回処理
	Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
}


//プレイヤーと敵との衝突処理
void Player::CollisionPlayerVsEnemies()
{
	EnemyManager& enemyManager = EnemyManager::Instance();

	// 全ての敵と総当たりで衝突処理
	int enemyCount = enemyManager.GetEnemyCount();
	for (int i = 0; i < enemyCount; ++i)
	{
		std::shared_ptr<Enemy> enemy = enemyManager.GetEnemy(i);
		if (!enemy || enemy->IsDestroyRequested()) continue;

		// 衝突処理
		DirectX::XMFLOAT3 outPosition;

		if (Collision::IntersectCylinderVsCylinder2(
			position,
			radius,
			height,
			enemy->GetPosition(),
			enemy->GetRadius(),
			enemy->GetHeight(),
			outPosition))
		{
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&position);
			DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&enemy->GetPosition());
			DirectX::XMVECTOR V = DirectX::XMVectorSubtract(P, E);
			DirectX::XMVECTOR N = DirectX::XMVector3Normalize(V);
			DirectX::XMFLOAT3 normal;
			DirectX::XMStoreFloat3(&normal, N);

			// 上から踏んづけた場合は小ジャンプする
			if (normal.y > 0.8f)
			{
				Jump(jumpSpeed * 0.5f);
				enemy->ApplyDamage(1, 0.5f);
			}
			else
			{
				enemy->SetPosition(outPosition);
			}

		}
	}
}

void Player::CollisionPlayerVsFences()
{
}


void Player::InputJump()
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	if (gamePad.GetButtonDown() & GamePad::BTN_A)
	{
		//Jump(jumpSpeed);
		//ジャンプ回数制限
		if (jumpCount < jumpLimit)
		{
			//ジャンプ
			jumpCount++;
			Jump(jumpSpeed);
		}
	}
}

void Player::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
	//基底クラスの関数呼び出し
	Character::RenderDebugPrimitive(rc, renderer);

	//弾丸デバッグプリミティブ描画
	projectileManager.RenderDebugPrimitive(rc, renderer);

	renderer->RenderCylinder(
		rc,
		GetPosition(),                           // 中心
		/*radius=*/GetRadius() + 0.2f,                         // 半径（ワールド単位：調整可）
		/*height=*/0.05f,                        // 薄い円柱でOK
		DirectX::XMFLOAT4(1, 1, 0, 0.5f));       // 色（半透明の黄）
	// --- 自動攻撃範囲の可視化 ---
	if (autoAttackEnabled)
	{
		renderer->RenderCylinder(
			rc,
			position,              // プレイヤーの位置を中心に
			autoAttackRange,       // 半径
			1.0f,                  // 高さ（見やすくするために低め）
			DirectX::XMFLOAT4(0, 1, 1, 1.0f) // 色：青・半透明
		);
	}
	// --- 選択中マーカー（薄い黄色リング） ---
	if (this == GetActivePtr()) {
		DirectX::XMFLOAT3 ringPos = position; ringPos.y += 0.02f;
		renderer->RenderCylinder(
			rc, ringPos, radius + 0.15f, 0.05f,
			DirectX::XMFLOAT4(1, 1, 0, 0.8f)
		);
	}

	// ★追加: 経路のデバッグ表示
	if (!currentPath.empty())
	{
		for (const auto& cell : currentPath)
		{
			DirectX::XMFLOAT3 pos = gridMap->GetWorldPosition(cell.first, cell.second);
			renderer->RenderSphere(rc, pos, 0.2f, DirectX::XMFLOAT4(0, 1, 0, 1)); // 緑の点
		}
	}
}

void Player::OnLanding()
{
	jumpCount = 0;
}

void Player::InputProjectile()
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	//直進弾丸発射
	if (gamePad.GetButtonDown() & GamePad::BTN_X)
	{
		// 前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);

		// 発射位置（プレイヤーの腰あたり）
		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + height * 0.5f;
		pos.z = position.z;


	}

	// 追尾弾丸発射
	if (gamePad.GetButtonDown() & GamePad::BTN_Y)
	{
		// 前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);

		// 発射位置（プレイヤーの腰あたり）
		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + height * 0.5f;
		pos.z = position.z;

		// ターゲット（デフォルトではプレイヤーの前方）
		DirectX::XMFLOAT3 target;
		target.x = pos.x + dir.x * 1000.0f;
		target.y = pos.y + dir.y * 1000.0f;
		target.z = pos.z + dir.z * 1000.0f;

		// 一番近くの敵をターゲットにする
		float dist = FLT_MAX;
		EnemyManager& enemyManager = EnemyManager::Instance();
		int enemyCount = enemyManager.GetEnemyCount();
		for (int i = 0; i < enemyCount; ++i)
		{
			// 敵との距離判定
			std::shared_ptr<Enemy> enemy = EnemyManager::Instance().GetEnemy(i);
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&position);
			DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&enemy->GetPosition());
			DirectX::XMVECTOR V = DirectX::XMVectorSubtract(E, P);
			DirectX::XMVECTOR D = DirectX::XMVector3LengthSq(V);
			float d;
			DirectX::XMStoreFloat(&d, D);
			if (d < dist)
			{
				dist = d;
				target = enemy->GetPosition();
				target.y += enemy->GetHeight() * 0.5f;
			}
		}

	}

}

// 弾丸と敵の衝突処理
void Player::CollisionProjectilesVsEnemies()
{
	EnemyManager& enemyManager = EnemyManager::Instance();

	const int projectileCount = projectileManager.GetProjectileCount();
	const int enemyCount = enemyManager.GetEnemyCount();

	for (int i = 0; i < projectileCount; ++i)
	{
		Projectile* projectile = projectileManager.GetProjectile(i);
		if (!projectile) continue;

		bool destroyed = false;

		// 2) つぎに「弾 × 敵」
		for (int j = 0; j < enemyCount; ++j)
		{
			std::shared_ptr<Enemy> enemy = enemyManager.GetEnemy(j);
			if (!enemy || enemy->IsDestroyRequested()) continue;
			if (!enemy) continue;

			DirectX::XMFLOAT3 outPosition;
			if (Collision::IntersectSphereVsCylinder(
				projectile->GetPosition(),
				projectile->GetRadius(),
				enemy->GetPosition(),
				enemy->GetRadius(),
				enemy->GetHeight(),
				outPosition))
			{
				if (enemy->ApplyDamage(1, 0.5f))
				{
					// 軽いノックバック（XZ）
					DirectX::XMFLOAT3 impulse{};
					const float power = 10.0f;
					const auto& e = enemy->GetPosition();
					const auto& p = projectile->GetPosition();
					float vx = e.x - p.x;
					float vz = e.z - p.z;
					float lenXZ = std::sqrt(vx * vx + vz * vz);
					if (lenXZ > 1e-4f) { vx /= lenXZ; vz /= lenXZ; }
					impulse.x = vx * power;
					impulse.y = power * 0.5f;
					impulse.z = vz * power;
					enemy->AddImpulse(impulse);
				}
				projectile->Destroy();
				break;
			}
		}
	}
}


// 自動攻撃（スライムのように一定間隔で自動発射）
void Player::AutoAttackUpdate(float elapsedTime)
{
	if (!autoAttackEnabled) return;

	// クールダウン
	if (autoAttackTimer > 0.0f) {
		autoAttackTimer -= elapsedTime;
		return;
	}

	// 発射位置（腰あたり）
	DirectX::XMFLOAT3 pos{ position.x, position.y + height * 0.5f, position.z };
	const float rangeSq = autoAttackRange * autoAttackRange;

	// ==============================
	// 1) ターゲット候補を検索
	// ==============================

	DirectX::XMFLOAT3 coreTargetPos{};
	bool coreInRange = false;
	DirectX::XMFLOAT3 enemyTargetPos{};
	bool enemyInRange = false;
	float nearestEnemyDistSq = FLT_MAX; // ★ 最も近い敵の距離（3D）

	// 1b. 敵のチェック (FindNearestEnemy を使わず、ここでループ)
	EnemyManager& em = EnemyManager::Instance();
	int count = em.GetEnemyCount();
	for (int i = 0; i < count; ++i)
	{
		std::shared_ptr<Enemy> e = em.GetEnemy(i);
		if (!e || e->IsDestroyRequested()) continue; // 削除フラグをチェック

		DirectX::XMFLOAT3 tempTargetPos = e->GetPosition();
		tempTargetPos.y += e->GetHeight() * 0.5f;

		float dx = tempTargetPos.x - pos.x;
		float dy = tempTargetPos.y - pos.y;
		float dz = tempTargetPos.z - pos.z;
		float distSq = dx * dx + dy * dy + dz * dz;

		// 射程内で、かつ、これまでで一番近い敵か？
		if (distSq <= rangeSq && distSq < nearestEnemyDistSq)
		{
			nearestEnemyDistSq = distSq;
			enemyTargetPos = tempTargetPos;
			enemyInRange = true;
		}
	}

	// ==============================
	// 2) 優先度に基づいて最終ターゲットを決定
	// ==============================
	DirectX::XMFLOAT3 finalTargetPos{};
	bool hasTarget = false;

	if (attackPriority == AttackPriority::CoreFirst)
	{
		if (coreInRange) {
			finalTargetPos = coreTargetPos;
			hasTarget = true;
		}
		else if (enemyInRange) {
			finalTargetPos = enemyTargetPos;
			hasTarget = true;
		}
	}
	else // (attackPriority == AttackPriority::EnemyFirst)
	{
		if (enemyInRange) {
			finalTargetPos = enemyTargetPos;
			hasTarget = true;
		}
		else if (coreInRange) {
			finalTargetPos = coreTargetPos;
			hasTarget = true;
		}
	}

	// ========= 3) 発射 =========
	if (hasTarget) {
		DirectX::XMFLOAT3 dir{
			finalTargetPos.x - pos.x,
			finalTargetPos.y - pos.y,
			finalTargetPos.z - pos.z
		};
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len < 1e-3f) return;
		dir.x /= len; dir.y /= len; dir.z /= len;



		autoAttackTimer = autoAttackInterval;
	}
}


bool Player::IsActive() const
{
	return sActive == this;
}

// 最寄りの敵を取得（XZ 平面距離）
std::shared_ptr<Enemy> Player::FindNearestEnemy() const
{
	int count = EnemyManager::Instance().GetEnemyCount();
	std::shared_ptr<Enemy> nearest = nullptr;
	float bestDist2 = FLT_MAX;

	for (int i = 0; i < count; ++i) {
		std::shared_ptr<Enemy> e = EnemyManager::Instance().GetEnemy(i);
		if (!e || e->IsDestroyRequested()) continue;
		if (!e) continue;
		const auto ep = e->GetPosition();
		float dx = ep.x - position.x;
		float dz = ep.z - position.z;
		float d2 = dx * dx + dz * dz;
		if (d2 < bestDist2) {
			bestDist2 = d2;
			nearest = e;
		}
	}
	return nearest;
}

// 非アクティブ時の自動移動（敵に向かい、近づきすぎたら停止）
void Player::UpdateAutoMoveToEnemy(float dt)
{
	std::shared_ptr<Enemy> target = FindNearestEnemy();
	if (!target) return;

	const auto tp = target->GetPosition();
	float vx = tp.x - position.x;
	float vz = tp.z - position.z;
	float dist = std::sqrt(vx * vx + vz * vz);
	if (dist < 1e-3f) return;

	vx /= dist; vz /= dist;

	// 常に敵の方向へ向く
	Turn(dt, vx, vz, turnSpeed * autoMoveTurnRate);

	// 一定距離より遠ければ前進、近ければ止まる
	if (dist > autoMoveStopDistance) {
		Move(dt, vx, vz, moveSpeed * autoMoveSpeedRate);
	}
	else {
		Move(dt, 0.0f, 0.0f, 0.0f);
	}
}

void Player::OnCollision(GameObject* object)
{
	//DirectX::XMFLOAT3 mtd = CollisionManager::Instance().GetMtd();
	if (fabsf(mtd.x) > 1e-6f || fabsf(mtd.y) > 1e-6f || fabsf(mtd.z) > 1e-6f)
	{
		position.x += mtd.x; position.y += mtd.y; position.z += mtd.z;
	}
}

// 攻撃優先度切り替え入力
void Player::InputToggleAttackPriority()
{
	// Input.h から GamePad インスタンスを取得
	Input& input = Input::Instance();
	GamePad& gamePad = input.GetGamePad();

	// 'X'キー（GamePad::BTN_B としてエミュレートされている）が押された瞬間
	if (gamePad.GetButtonDown() & GamePad::BTN_B)
	{
		if (attackPriority == AttackPriority::CoreFirst)
		{
			attackPriority = AttackPriority::EnemyFirst;
		}
		else
		{
			attackPriority = AttackPriority::CoreFirst;
		}
	}
}

bool Player::UpdateActiveByKeyboard(const std::vector<std::unique_ptr<Player>>& players)
{
	// キーボードの '1' ～ '5' をチェック
	// '1' の仮想キーコードは 0x31 なので、ループで回せます
	for (int i = 0; i < 5; ++i)
	{
		// 押されているかチェック ('1' + i)
		if (GetAsyncKeyState('1' + i) & 0x8000)
		{
			// その番号のプレイヤーが存在するか確認
			// (i=0なら1人目, i=4なら5人目)
			if (i < players.size())
			{
				Player::SetActive(players[i].get());
				return true; // 切り替え発生
			}
		}
	}

	// ゲームパッドでの切り替え（バックアップとして残す場合）
	// BACKボタン -> 1人目, STARTボタン -> 2人目
	auto& gp = Input::Instance().GetGamePad();
	if ((gp.GetButtonDown() & GamePad::BTN_BACK) && !players.empty())
	{
		Player::SetActive(players[0].get());
		return true;
	}
	if ((gp.GetButtonDown() & GamePad::BTN_START) && players.size() >= 2)
	{
		Player::SetActive(players[1].get());
		return true;
	}

	return false;
}

// ... (前略)
#include "Core.h" // Coreクラスを使うために必要（既にインクルードされているはずですが確認）

// ... (中略)

void Player::UpdateSpawn(std::vector<std::unique_ptr<Player>>& players, const Picking_Ray& pickingRay)
{
	// 左クリック判定
	bool isClick = (Input::Instance().GetMouse().GetButtonDown() & Mouse::BTN_LEFT);

	// ImGui操作中でなく、かつクリックされた場合
	if (!ImGui::GetIO().WantCaptureMouse && isClick)
	{
		// 5体未満の場合のみ生成
		if (players.size() < 5)
		{
			DirectX::XMFLOAT3 rayOrg = pickingRay.GetRayOrigin();
			DirectX::XMFLOAT3 rayDir = pickingRay.GetRayDirection();

			// レイが下向き（地面方向）かチェック
			if (fabs(rayDir.y) > 1e-4f)
			{
				// 地面(Y=0)との交差計算
				float t = -rayOrg.y / rayDir.y;
				if (t > 0.0f)
				{
					float hitX = rayOrg.x + t * rayDir.x;
					float hitZ = rayOrg.z + t * rayDir.z;

					// ===========================================================
					// ★追加: コア（拠点中心）との距離チェック
					// ===========================================================
					Core* core = Core::Instance();
					if (core)
					{
						// クリック地点とコアの距離の二乗を計算
						float dx = hitX - core->position.x;
						float dz = hitZ - core->position.z;
						float distSq = dx * dx + dz * dz;

						// 壁の内側とみなす「禁止エリアの半径」
						// ※画面上の壁の配置に合わせて数値を調整してください（例: 20.0f ～ 30.0f くらい）
						float forbiddenRadius = 30.0f;

						// 指定半径より内側なら、何もせずリターン（生成しない）
						if (distSq < forbiddenRadius * forbiddenRadius)
						{
							return;
						}
					}
					// ===========================================================

					// 生成と初期化
					auto newPlayer = std::make_unique<Player>();
					newPlayer->Initialize();
					newPlayer->SetPosition({ hitX, 0.0f, hitZ });

					Player* rawPtr = newPlayer.get();
					players.push_back(std::move(newPlayer));

					// 1体目なら自動的に操作キャラにする
					if (players.size() == 1)
					{
						Player::SetActive(rawPtr);
					}
				}
			}
		}
	}
}