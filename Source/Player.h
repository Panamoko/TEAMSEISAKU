#pragma once

#include "Character.h"
#include <vector>
#include <memory>
#include "Animator.h"
#include "AStar.h"
#include "System/Sprite.h"

class Enemy;
class Picking_Ray;
class Model;
class GridMap;
class SceneGame;

// プレイヤー（基底クラス）
class Player : public Character
{
public:
	Player();
	~Player() override;

	// --- 共通の静的管理機能 ---
	static Player& Instance();
	static void SetActive(Player* p);
	static Player* GetActivePtr();
	static void RegisterPlayer(Player* player);
	static void UnregisterPlayer(Player* player);
	static const std::vector<Player*>& GetAllPlayers();

	// --- 基本機能 ---
	// ★修正: 派生クラスでオーバーライドするために virtual をつける
	virtual void Initialize();

	void Finalize();
	void Update(float elapsedTime) override;
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
	void RenderUI(const RenderContext& rc, float x, float y, float size = 128.0f);
	void OnCollision(GameObject* object) override;

	// --- 派生クラスでオーバーライドする味方生成関数 ---
	virtual void SpawnAlly(SceneGame* scene);

	// --- 共通設定 ---
	void SetGridMap(const GridMap* map) { gridMap = map; }
	void RequestPathRecalculation();
	bool IsPlayerActive() const;

	// SceneGameから呼べるように public に移動
	bool UpdateAutoSpawn(float elapsedTime);

	// shared_ptr に統一して扱いやすくする
	static void UpdateSpawn(std::vector<std::shared_ptr<Character>>& players, const Picking_Ray& pickingRay);

	// キーボード切り替え用
	static bool UpdateActiveByKeyboard(const std::vector<std::shared_ptr<Character>>& players);

protected:
	// 共通初期化用関数
	void InitializeCommon(const char* modelPath, const char* iconPath);

	void OnLanding() override;
	void OnDead() override;

	// 入力・移動
	void InputMove(float elapsedTime);
	void InputJump();
	DirectX::XMFLOAT3 GetMoveVec() const;

	// AI・自動生成
	void UpdateMoveToCore(float elapsedTime);

protected:
	Model* model = nullptr;
	Animator animator;

	// パラメータ
	float moveSpeed = 5.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);
	float jumpSpeed = 12.0f;
	int jumpCount = 0;
	int jumpLimit = 2;

	// ボーン情報
	int headBoneIndex = -1;
	int crownNodeIndex = -1;

	// 自動スポーン設定
	float spawnTimer = 0.0f;
	float spawnInterval = 3.0f;

	// AI・経路探索
	const GridMap* gridMap = nullptr;
	AStar aStar;
	std::vector<std::pair<int, int>> currentPath;
	int pathIndex = 0;
	float pathRecalcTimer = 0.0f;
	float autoMoveSpeedRate = 0.8f;
	float autoMoveTurnRate = 1.0f;

	// UI
	Sprite* playerIcon = nullptr;
	Sprite* hpBarSprite = nullptr;
	CylinderCollider* cylinder = nullptr;

private:
	static Player* sActive;
	static std::vector<Player*> sAllPlayers;
};