#pragma once

#include "System/ModelRenderer.h"
#include "character.h"
#include "ProjectileManager.h"
#include <vector>
#include <memory>
#include"Animator.h"
#include "AStar.h" // ★追加

class Enemy;
class Picking_Ray;
class Model;
class GridMap; // ★前方宣言

// プレイヤー
class Player : public Character
{
public:
	Player() {};
	~Player() override {};

	// アクティブ個体の取得／設定
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
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	// UI描画処理 (2D)
	// GameSpriteを使わず、Player自身がSpriteを描画します
	// 表示位置 (x, y) を指定できるように引数を追加
	void RenderUI(const RenderContext& rc, float x, float y, float size = 128.0f);

	//デバッグ用GUI描画
	void DrawDebugGUI();

	// ジャンプ入力処理
	void InputJump();

	//デバッグプリミティブ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

	// 自分がアクティブか判定
	bool IsActive() const;

	// 数字キーで操作プレイヤーを切り替える
	static bool UpdateActiveByKeyboard(const std::vector<std::unique_ptr<Player>>& players);

	// マウス入力によるスポーン処理
	static void UpdateSpawn(std::vector<std::unique_ptr<Player>>& players, const Picking_Ray& pickingRay);

	//GridMapをセット
	void SetGridMap(const GridMap* map) { gridMap = map; }

	//外部から経路再計算を要求するための関数
	void RequestPathRecalculation();

protected:
	//着地したときに呼ばれる
	void OnLanding() override;

private:
	// スティック入力値から移動ベクトルを取得
	DirectX::XMFLOAT3 GetMoveVec() const;

	// 移動入力処理
	void InputMove(float elapsedTime);

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

	// ★追加: コアへの自動移動更新
	void UpdateMoveToCore(float elapsedTime);

	//衝突処理
	void OnCollision(GameObject* object) override;

private:
	Model* model = nullptr;
	float		moveSpeed = 5.0f;
	float		turnSpeed = DirectX::XMConvertToRadians(720);

	// ★移動: ここに2つまとめておくと管理しやすいです
	int headBoneIndex = -1;   // 頭のボーンの番号
	int crownNodeIndex = -1;  // 王冠のノードの番号

	std::shared_ptr<Enemy> FindNearestEnemy() const;
	Animator animator;
	enum class AttackPriority {
		CoreFirst,  // コア優先
		EnemyFirst  // スライム優先
	};
	AttackPriority attackPriority = AttackPriority::CoreFirst;

	float jumpSpeed = 12.0f;
	int jumpCount = 0;
	int jumpLimit = 2;
	ProjectileManager projectileManager;

	// --- 自動攻撃設定 ---
	bool  autoAttackEnabled = true;     // 自動攻撃ON/OFF
	float autoAttackRange = 8.0f;     // 索敵半径（m）
	float autoAttackInterval = 1.5f;     // 発射間隔（秒）
	float autoAttackTimer = 0.0f;     // タイマー

	// 調整用パラメータ
	bool  autoMoveToEnemyEnabled = true;
	float autoMoveSpeedRate = 0.8f;
	float autoMoveTurnRate = 1.0f;
	float autoMoveStopDistance = 3.0f;

	// 現在アクティブなプレイヤー
	static Player* sActive;

	// 全プレイヤーのリスト
	static std::vector<Player*> sAllPlayers;

	CylinderCollider* cylinder;

	//経路探索用
	const GridMap* gridMap = nullptr;
	AStar aStar;
	std::vector<std::pair<int, int>> currentPath;
	int pathIndex = 0;
	float pathRecalcTimer = 0.0f;
	bool once = true;

	//UI表示用スプライトのポインタ
	Sprite* playerIcon = nullptr;

};