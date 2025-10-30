#pragma once
#include "Stage.h"
#include "Player.h"
#include "CameraController.h"
#include "Scene.h"
#include "Editor.h"
#include <memory>
#include <vector>
#include "AllySlime.h"
#include <cfloat>   // ← 追加（FLT_MAX 用）
// ゲームシーン
//class SceneGame
class SceneGame : public Scene
{
public:
	SceneGame() {}
	//~SceneGame() {};
	~SceneGame() override {}	//Sceneクラスを継承し、overrideキーワードを付ける

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

private:
	editor game_editor;
	std::vector<std::unique_ptr<Model>> models;

	std::vector<std::unique_ptr<GameObject>> objects;
	std::vector<std::unique_ptr<SpriteObject>> sprites2d;

	Stage* stage = nullptr;

	std::vector<std::unique_ptr<Player>> players;
	std::vector<std::unique_ptr<AllySlime>> allies;

	

	CameraController* cameraController = nullptr;
};
