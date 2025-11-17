#pragma once
#include "Stage.h"
#include "Player.h"
#include "CameraController.h"
#include "Scene.h"
#include "Editor.h"
#include <memory>
#include <vector>
#include "AllySlime.h"
#include "AllySlimeHoming.h"
#include "System/ModelRenderer.h"
#include <cfloat>   // ← 追加（FLT_MAX 用）
#include "GridMap.h"

class Player;
class AllySlime;         // 既存＝直線弾
class SceneGame : public Scene
{
public:
	SceneGame() = default;
	//~SceneGame() {};
	~SceneGame() override;	//Sceneクラスを継承し、overrideキーワードを付ける

	// 初期化
	//void Initialize();
	void Initialize() override;

	// 終了化
	//void Finalize();
	void Finalize() override;

	// 更新処理
	//void Update(float elapsedTime);
	void Update(float elapsedTime) override;

	// 描画処理
	//void Render();
	void Render() override;

	// GUI描画
	//void DrawGUI();
	void DrawGUI() override;

	// スローモーション設定
	static void SetSlowMotion(float scale, float duration);

	// 現在の時間スケールを取得 
	static float GetTimeScale() { return s_timeScale; }

private:

	editor game_editor;
	std::vector<std::unique_ptr<Model>> models;
	// ★直線／追尾の別管理
	std::vector<std::unique_ptr<AllySlime>>       alliesStraight;
	std::vector<std::unique_ptr<AllySlimeHoming>> alliesHoming;
	std::vector<std::shared_ptr<GameObject>> objects;
	std::vector<std::unique_ptr<SpriteObject>> sprites2d;

	Stage* stage = nullptr;
	GridMap grid_map;
	// 追加ヘルパ

	void AddAllyStraightFor(Player* leader);
	void AddAllyHomingFor(Player* leader);

	std::vector<std::unique_ptr<Player>> players;
	std::vector<std::unique_ptr<AllySlime>> allies;

	int  CountAlliesFor(Player* leader) const;
	int CountAlliesGlobal() const;

	CameraController* cameraController = nullptr;

	// スロー管理用
	static float s_timeScale;    // 現在の時間倍率 (1.0f が通常)
	static float s_slowTimer;    // スロー解除までの実時間（秒）
};
