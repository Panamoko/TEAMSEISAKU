#pragma once

#include "Character.h"
#include <vector>
#include <memory>
#include <string> // ★追加: stringを使うため
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

	static void ResetSpawnCount();

	// 現在の生成回数を取得する関数
	static int GetSpawnCount() { return s_spawnCount; }

	virtual void Initialize();

	void Finalize();
	void Update(float elapsedTime) override;
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;
	void RenderUI(const RenderContext& rc, float x, float y, float size = 128.0f);
	void OnCollision(GameObject* object) override;

	virtual void SpawnAlly(SceneGame* scene);

	void SetGridMap(const GridMap* map) { gridMap = map; }
	void RequestPathRecalculation();
	bool IsPlayerActive() const;

	bool UpdateAutoSpawn(float elapsedTime);

	static void UpdateSpawn(std::vector<std::shared_ptr<Character>>& players, const Picking_Ray& pickingRay);
	static bool UpdateActiveByKeyboard(const std::vector<std::shared_ptr<Character>>& players);

protected:
	void InitializeCommon(const char* modelPath, const char* iconPath);

	void OnLanding() override;
	void OnDead() override;

	void InputMove(float elapsedTime);
	DirectX::XMFLOAT3 GetMoveVec() const;

	bool UpdateMoveToCore(float elapsedTime);

protected:
	Model* model = nullptr;
	Animator animator;

	// ★追加: 現在再生中のアニメーション名を記録する変数
	std::string currentAnim = "";

	// パラメータ
	float moveSpeed = 5.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);
	float jumpSpeed = 12.0f;
	int jumpCount = 0;
	int jumpLimit = 2;

	int headBoneIndex = -1;
	int crownNodeIndex = -1;

	float spawnTimer = 0.0f;
	float spawnInterval = 3.0f;

	const GridMap* gridMap = nullptr;
	AStar aStar;
	std::vector<std::pair<int, int>> currentPath;
	int pathIndex = 0;
	float pathRecalcTimer = 0.0f;
	float autoMoveSpeedRate = 0.8f;
	float autoMoveTurnRate = 1.0f;

	Sprite* playerIcon = nullptr;
	Sprite* hpBarSprite = nullptr;
	CylinderCollider* cylinder = nullptr;

private:
	static Player* sActive;
	static std::vector<Player*> sAllPlayers;

	static int s_spawnCount;
};