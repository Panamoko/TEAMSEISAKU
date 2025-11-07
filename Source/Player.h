#pragma once

#include "System/ModelRenderer.h"
#include "character.h"
#include "ProjectileManager.h"
#include <vector>
#include <memory>
#include"Animator.h"
class Enemy;// ← 追記（前方宣言）ほぼ引数用のポインタ取得

// プレイヤー
class Player : public Character
{
public:
	Player() {};
	~Player() override {};

    // アクティブ個体の取得／設定（“選択した方”を操作するため）
    static Player& Instance();
    static void SetActive(Player* p);
    static Player* GetActivePtr();

	// 全プレイヤーインスタンスの管理用
	static void RegisterPlayer(Player* player);
	static void UnregisterPlayer(Player* player);
	static const std::vector<Player*>& GetAllPlayers();

	//初期化
	void Initialize();

	//終了化
	void Finalize();
	
	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc,ModelRenderer* renderer);

	//デバッグ用GUI描画
	void DrawDebugGUI();

	// ジャンプ入力処理
	void InputJump();

	//デバッグプリミティブ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

	// 自分がアクティブか判定
	bool IsActive() const;        

	// 数字キーで操作プレイヤーを切り替える（1→先頭、2→2人目）
	// 戻り値: 切り替えが発生したら true
	static bool UpdateActiveByKeyboard(const std::vector<std::unique_ptr<Player>>& players);

protected:
	//着地したときに呼ばれる
	void OnLanding() override;

private:
	// スティック入力値から移動ベクトルを取得
	DirectX::XMFLOAT3 GetMoveVec() const;

	//// 移動処理
	//void Move(float elapsedTime, float vx, float vz, float speed);

	// 移動入力処理
	void InputMove(float elapsedTime);

	//// 旋回処理
	//void Turn(float elapsedTime, float vx, float vz, float speed);

	//プレイヤーとエネミーとの衝突処理
	void CollisionPlayerVsEnemies();

	//プレイヤーと柵との衝突処理
	void CollisionPlayerVsFences();

	//弾丸入力処理
	void InputProjectile();

	// 弾丸と敵の衝突処理
	void CollisionProjectilesVsEnemies();

	// 自動攻撃の更新処理
	void AutoAttackUpdate(float elapsedTime);

	//攻撃の優先度
	void InputToggleAttackPriority();

	// 自動移動更新
	void UpdateAutoMoveToEnemy(float dt);  

	//衝突処理
	void OnCollision(GameObject* object) override;

private:
	Model* model = nullptr;
	float		moveSpeed = 5.0f;
	float		turnSpeed = DirectX::XMConvertToRadians(720);

	std::shared_ptr<Enemy> FindNearestEnemy() const;         // ← 追記：最寄り敵の検索
	Animator animator;
	enum class AttackPriority {
		CoreFirst,  // コア優先（現在の動作）
		EnemyFirst  // スライム優先
	};
	AttackPriority attackPriority = AttackPriority::CoreFirst;

	float jumpSpeed = 12.0f;
	//float gravity = -30.0f;
	//DirectX::XMFLOAT3 velocity = {0,0,0};
	int jumpCount = 0;
	int jumpLimit = 2;
	ProjectileManager projectileManager;

	// --- 自動攻撃設定 ---
    bool  autoAttackEnabled = true;     // 自動攻撃ON/OFF
    float autoAttackRange   = 8.0f;     // 索敵半径（m）
    float autoAttackInterval= 1.5f;     // 発射間隔（秒）
    float autoAttackTimer   = 0.0f;     // タイマー

	// 調整用パラメータ（必要ならGUIでいじれるように）
	bool  autoMoveToEnemyEnabled = true; // ← 追記：自動追尾ON/OFF
	float autoMoveSpeedRate = 0.8f; // ← 追記：通常移動に対する倍率
	float autoMoveTurnRate = 1.0f; // ← 追記：通常旋回に対する倍率
	float autoMoveStopDistance = 3.0f; // ← 追記：これ未満で停止

	// 現在アクティブなプレイヤー（実体は Player.cpp で定義）
    static Player* sActive;

	// 全プレイヤーのリスト (実体は Player.cpp で定義)
	static std::vector<Player*> sAllPlayers;

	CylinderCollider* cylinder;
};
