#include "EnemySlimeMelee.h"
#include "Player.h" // Player にダメージを与えるために include
#include "MathUtils.h" // 距離計算用 (sqrtf)
#include "ProjectileStraite.h"

// コンストラクタ
EnemySlimeMelee::EnemySlimeMelee()
{
	// class_name を上書き (デバッグ用)
	class_name = "EnemySlimeMelee";

	// ※モデルやスケール、半径などは基底クラス(EnemySlime)の
	//   コンストラクタで設定されたものをそのまま使います。
	//   もしモデルを変えたい場合は、ここで再ロードします。
	//   model = ModelManager::Instance().Load("Data/Model/Slime/SlimeMelee.mdl");
}

// デストラクタ
EnemySlimeMelee::~EnemySlimeMelee()
{
}

//攻撃ステートへ偏移 (オーバーライド)
void EnemySlimeMelee::SetAttackState(Player* target)
{
	// 基底クラス(EnemySlime)の SetAttackState を呼び出す
	// ( state = State::Attack; や targetPlayer = target; を実行してもらう)
	EnemySlime::SetAttackState(target);

	// 近距離攻撃用のタイマーをセット
	stateTimer = 0.5f; // 最初の一撃は早めにするなど
}

//攻撃ステート更新処理 (オーバーライド)
void EnemySlimeMelee::UpdateAttackState(float elapsedTime)
{
	// ターゲットが有効かチェック (基底クラスと同様)
	if (!targetPlayer)
	{
		SetIdleState(); // アイドル状態に戻る
		return;
	}

	//目標地点を "記憶したターゲットの" 位置に設定
	targetPosition = targetPlayer->GetPosition();

	// ★★★ 近距離AIのコアロジック ★★★

	// 1. プレイヤーとのXZ平面の距離を計算
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float distSq = vx * vx + vz * vz; // 2乗距離

	// 2. プレイヤーとの距離に応じて行動を決定
	if (distSq > (attackRange * attackRange))
	{
		// 2a. 攻撃範囲(attackRange)の外にいる場合 -> プレイヤーに接近する
		//     (移動速度 1.0 = 100% で移動)
		MoveToTarget(elapsedTime, 1.0f, 1.0f);
	}
	else
	{
		// 2b. 攻撃範囲(attackRange)の内側にいる場合 -> 停止して攻撃準備
		//     (旋回のみ行う)
		MoveToTarget(elapsedTime, 0.0f, 1.0f);

		// 3. 攻撃タイマー処理
		stateTimer -= elapsedTime;
		if (stateTimer < 0.0f)
		{
			// ★ タイマー満了: プレイヤーにダメージを与える
			// (Player::ApplyDamage の存在を仮定)
			const float invincibleTime = 0.5f; // ダメージ後の無敵時間
			targetPlayer->ApplyDamage(attackDamage, invincibleTime);

			//前方向
			DirectX::XMFLOAT3 dir;
			dir.x = sinf(angle.y);
			dir.y = 0.0f;
			dir.z = cosf(angle.y);
			//発射位置(プレイヤーの腰あたり)
			DirectX::XMFLOAT3 pos;
			pos.x = position.x;
			pos.y = position.y + height * 0.5f;
			pos.z = position.z;
			//発射
			ProjectileStraite* projectile = new ProjectileStraite(&projectileManager);
			projectile->Launch(dir, pos);

			// ★ タイマーリセット (次の攻撃間隔)
			stateTimer = attackInterval;
		}
	}


	// 4. 索敵範囲(searchRange)外に出たら見失う (基底クラスと同様)
	// ※ 3D距離で判定 (基底クラスのロジック と合わせる)
	float vy = targetPosition.y - position.y;
	float distSq3D = (vx * vx + vy * vy + vz * vz);

	if (distSq3D > (searchRange * searchRange))
	{
		//プレイヤーを見失ったら待機ステートへ
		SetIdleState();
		targetPlayer = nullptr; // ターゲットクリア
	}
}

// ゲームオブジェクトとして登録
REGISTER_GAMEOBJECT(EnemySlimeMelee);