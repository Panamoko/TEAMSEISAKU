#pragma once
#include "Stage.h"
#include "Player.h" // Player.hをインクルード
#include "Character.h"
#include "CameraController.h"
#include "Scene.h"
#include "Editor.h"
#include <memory>
#include <vector>
#include "AllySlime.h"
#include "AllySlimeHeal.h"
#include "System/ModelRenderer.h"
#include <cfloat>
#include "GridMap.h"
#include "Picking_Ray.h"
#include "AllySlimeMelee.h"
#include "GameSprite.h"
#include "System/RenderTarget.h"

// 派生クラスのインクルード
#include "PlayerMelee.h"
#include "PlayerHeal.h"
#include "PlayerShot.h"

class SceneGame : public Scene
{
public:
	SceneGame(const std::string& name = "scene_play") { scene_name = name; };
	~SceneGame() override;

	void Initialize() override;
	void Finalize() override;
	void Update(float elapsedTime) override;
	void Render() override;
	void DrawGUI() override;

	static void SetSlowMotion(float scale, float duration);
	static float GetTimeScale() { return s_timeScale; }

	void AddAllyStraightFor(Player* leader);
	void AddAllyHomingFor(Player* leader);
	void AddAllyMeleeFor(Player* leader);

private:
	void RenderPiP(ID3D11DeviceContext* dc);
	bool UpdatePiP();
	void UpdatePlayerSpawn(); // スポーン処理用

	std::vector<std::unique_ptr<AllySlimeMelee>> alliesMelee;

	editor game_editor;
	std::vector<std::unique_ptr<Model>> models;
	std::vector<std::unique_ptr<AllySlime>>       alliesStraight;
	std::vector<std::unique_ptr<AllySlimeHeal>> alliesHoming;
	std::vector<std::shared_ptr<GameObject>> objects;
	std::vector<std::unique_ptr<GameSprite>> sprites2d;

	Stage* stage = nullptr;
	GridMap grid_map;
	Picking_Ray pickingRay;

	std::vector<std::shared_ptr<Character>> players;

	int  CountAlliesFor(Player* leader) const;
	int CountAlliesGlobal() const;

	CameraController* cameraController = nullptr;

	static float s_timeScale;
	static float s_slowTimer;

	RenderTarget* pipRenderTarget = nullptr;
	bool isPipExpanded = false;
	Sprite* pipFrameSprite = nullptr;
};