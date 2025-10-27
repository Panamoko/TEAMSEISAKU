#include "Player.h"
#include "System/Input.h"
#include <imgui.h>
#include "Camera.h"
#include "EnemyManager.h"
#include "Collision.h"
#include "ProjectileStraite.h"
#include "ProjectileHoming.h"

Player* Player::sActive = nullptr;

Player& Player::Instance() { return *sActive; }
void Player::SetActive(Player* p) { sActive = p; }
Player* Player::GetActivePtr() { return sActive; }

//初期化
void Player::Initialize()
{
	model = new Model("Data/Model/Mr.Incredible/Mr.Incredible.mdl");

	// モデルが大きいのでスケーリング
	scale.x = scale.y = scale.z = 0.01f;
}

//終了化
void Player::Finalize()
{
	delete model;
}

void Player::Update(float elapsedTime)
{
     const bool isActive = (this == GetActivePtr());
     if (isActive) {
         // 入力は“選択された個体のみ”
         InputMove(elapsedTime);
         InputJump();
         InputProjectile();
         AutoAttackUpdate(elapsedTime);   // ← 非アクティブでも撃たせたいなら下へ移動
     } else {
         // 非アクティブでも自動攻撃させたいならこちらで呼ぶ
         AutoAttackUpdate(elapsedTime);
     }

	//速力更新処理
	UpdateVelocity(elapsedTime);

	//弾丸更新処理
	projectileManager.Update(elapsedTime);

	//プレイヤーと敵との衝突処理
	CollisionPlayerVsEnemies();

	//弾丸と敵の衝突処理
	CollisionProjectilesVsEnemies();

	// オブジェクト行列を更新
	UpdateTransform();

	// モデル行列更新
	model->UpdateTransform();
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

			// 回転値
			//ImGui::InputFloat("rot", &rot);

		}
		 // 自動攻撃のデバッグ設定
         if (ImGui::CollapsingHeader("Auto Attack", ImGuiTreeNodeFlags_DefaultOpen))
         {
             ImGui::Checkbox("Enabled", &autoAttackEnabled);
             ImGui::DragFloat("Range", &autoAttackRange, 0.1f, 0.0f, 100.0f);
             ImGui::DragFloat("Interval (sec)", &autoAttackInterval, 0.01f, 0.1f, 10.0f);
             // 現在のタイマー表示
             ImGui::Text("Timer: %.2f", autoAttackTimer);
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
		Enemy* enemy = enemyManager.GetEnemy(i);

		// 衝突処理
		DirectX::XMFLOAT3 outPosition;
/*
		if (Collision::IntersectSphereVsSphere(
			position, radius,
			enemy->GetPosition(),
			enemy->GetRadius(),
			outPosition))
		{
			// 押し出し後の位置設定
			enemy->SetPosition(outPosition);
		}
*/

		if (Collision::IntersectCylinderVsCylinder(
			position, 
			radius,
			height,
			enemy->GetPosition(),
			enemy->GetRadius(),
			enemy->GetHeight(),
			outPosition))
		{
			// 押し出し後の位置設定
			//enemy->SetPosition(outPosition);
						
			// 敵の真上付近に当たったかを判定
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&position);
			DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&enemy->GetPosition());
			DirectX::XMVECTOR V = DirectX::XMVectorSubtract(P, E);
			DirectX::XMVECTOR N = DirectX::XMVector3Normalize(V);
			DirectX::XMFLOAT3 normal;
			DirectX::XMStoreFloat3(&normal, N);

			// 上から踏んづけた場合は小ジャンプする
			if (normal.y > 0.8f)
			{
				// 小ジャンプする
				Jump(jumpSpeed * 0.5f);

				//踏みつけ処理も実装しているので、ダメージ処理を実装してみる。
				//enemy->ApplyDamage(1);
				enemy->ApplyDamage(1,0.5f);
			}
			else
			{
				// 押し出し後の位置設定
				enemy->SetPosition(outPosition);
			}

		}
	}
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

	 // --- 自動攻撃範囲の可視化 ---
    if (autoAttackEnabled)
    {
        // 攻撃範囲を円柱で表示（スライムの索敵円柱を参考に）:contentReference[oaicite:1]{index=1}
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

		// 発射
		//ProjectileStraite* projectile = new ProjectileStraite();
		ProjectileStraite* projectile = new ProjectileStraite(&projectileManager);
		projectile->Launch(dir, pos);

		//弾丸クラスのコンストラクタで呼び出すようになったので削除
		//projectileManager.Register(projectile);
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
			Enemy* enemy = EnemyManager::Instance().GetEnemy(i);
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

		// 発射
		ProjectileHoming* projectile = new ProjectileHoming(&projectileManager);
		projectile->Launch(dir, pos, target);
	}

}

// 弾丸と敵の衝突処理
void Player::CollisionProjectilesVsEnemies()
{
	EnemyManager& enemyManager = EnemyManager::Instance();

	// 全ての弾丸と全ての敵を総当たりで衝突処理
	int projectileCount = projectileManager.GetProjectileCount();
	int enemyCount = enemyManager.GetEnemyCount();
	for (int i = 0; i < projectileCount; ++i)
	{
		Projectile* projectile = projectileManager.GetProjectile(i);

		for (int j = 0; j < enemyCount; ++j)
		{
			Enemy* enemy = enemyManager.GetEnemy(j);

			// 衝突処理
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
					// 吹き飛ばす
					{
						DirectX::XMFLOAT3 impulse;
						
						const float power = 10.0f;
						const DirectX::XMFLOAT3& e = enemy->GetPosition();
						const DirectX::XMFLOAT3& p = projectile->GetPosition();
						float vx = e.x - p.x;
						float vz = e.z - p.z;
						float lengthXZ = sqrtf(vx * vx + vz * vz);
						vx /= lengthXZ;
						vz /= lengthXZ;

						impulse.x = vx * power;
						impulse.y = power * 0.5f;
						impulse.z = vz * power;

						enemy->AddImpulse(impulse);
					}

					//弾丸破棄
					projectile->Destroy();
				}
			}
		}
	}
}

// 自動攻撃（スライムのように一定間隔で自動発射）
void Player::AutoAttackUpdate(float elapsedTime)
{
	if (!autoAttackEnabled) return;

    // タイマー更新
    if (autoAttackTimer > 0.0f) {
        autoAttackTimer -= elapsedTime;
        return;
    }

    // 一番近い敵を探索（既存のYボタン追尾弾のロジックと同様の探索を流用）:contentReference[oaicite:5]{index=5}
    EnemyManager& enemyManager = EnemyManager::Instance();
    int enemyCount = enemyManager.GetEnemyCount();
    if (enemyCount <= 0) return;

    float bestDistSq = FLT_MAX;
    Enemy* bestEnemy = nullptr;

    for (int i = 0; i < enemyCount; ++i)
    {
        Enemy* enemy = enemyManager.GetEnemy(i);

        // 3D距離（中心対中心）
        const DirectX::XMFLOAT3& epos = enemy->GetPosition();
        float dx = epos.x - position.x;
        float dy = epos.y - position.y;
        float dz = epos.z - position.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        // 索敵半径内のみ対象
        if (distSq <= (autoAttackRange * autoAttackRange))
        {
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                bestEnemy = enemy;
            }
        }
    }

    if (!bestEnemy) return;

    // 発射方向：腰位置から敵の胴体中心（高さの 0.5）を狙う（スライムの発射実装と整合）:contentReference[oaicite:6]{index=6}:contentReference[oaicite:7]{index=7}
    DirectX::XMFLOAT3 pos;  // 発射位置（プレイヤーの腰あたり）
    pos.x = position.x;
    pos.y = position.y + height * 0.5f;
    pos.z = position.z;

    DirectX::XMFLOAT3 target = bestEnemy->GetPosition();
    target.y += bestEnemy->GetHeight() * 0.5f;

    // 方向ベクトル計算＆正規化
    DirectX::XMFLOAT3 dir;
    dir.x = target.x - pos.x;
    dir.y = target.y - pos.y;
    dir.z = target.z - pos.z;
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    // 直進弾を発射（Swordモデルを使う既存弾）:contentReference[oaicite:8]{index=8}
    auto* projectile = new ProjectileStraite(&projectileManager);
    projectile->Launch(dir, pos);

    // 次回までのインターバル再設定（スライム攻撃ステートのタイマー挙動に近い考え方）:contentReference[oaicite:9]{index=9}
    autoAttackTimer = autoAttackInterval;
}