#pragma once

#include "System/Sprite.h"
#include "Scene.h"
#include "Editor.h"
#include "ChangeSceneSytem.h"
#include "Dissolve.h"
#include <System/AudioSource.h>

// 3D表示用
#include "Camera.h"
#include "Player.h"
#include "AllySlimeMelee.h"

// タイトルシーン
class SceneTitle : public Scene
{
public:
	SceneTitle();
	~SceneTitle() override {}

	void Initialize() override;
	void Finalize() override;
	void Update(float elapsedTime) override;
	void Render() override;
	void DrawGUI() override;

private:
	// タイトル画面用の2D要素
	Sprite* sprite = nullptr;      // タイトルロゴ
	Sprite* spriteStart = nullptr; // Game Startボタン
	Sprite* spriteTutorial = nullptr; // Tutorialボタン

	// エディタ・オブジェクト管理
	editor game_editor;
	std::vector<std::shared_ptr<GameObject>> objects;
	std::vector<std::unique_ptr<GameSprite>> sprites;

	// 3D演出用オブジェクト
	std::unique_ptr<Player> player;
	std::vector<std::shared_ptr<AllySlimeMelee>> slimes;

	// 背景（地面）用モデル
	Model* groundModel = nullptr;
	DirectX::XMFLOAT4X4 groundTransform; // 地面の行列

	// ボタンの座標とサイズ
	DirectX::XMFLOAT3 startButtonPos;
	DirectX::XMFLOAT3 startButtonSize;
	DirectX::XMFLOAT3 tutorialButtonPos;
	DirectX::XMFLOAT3 tutorialButtonSize;

	// ボタンのホバー状態管理
	bool isStartHover = false;
	bool isTutorialHover = false;

	// 点滅演出用
	float alpha_timer = 0.0f;
	float blink_interval = 0.5f;
	DirectX::XMFLOAT4 render_color = { 1.0f, 1.0f, 1.0f, 1.0f };

	// カメラ演出用タイマー
	float cameraTimer = 0.0f;

	std::unique_ptr<ChangeSceneSytem> change_scene;

	// ディゾルブ制御用（画面遷移演出）
	std::unique_ptr<Dissolve> dissolve;
	bool isSceneChanging = false;   // 遷移開始フラグ
	float transitionTimer = 0.0f;   // タイマー
	const float transitionDuration = 1.5f; // 暗転にかかる時間

	// 遷移先のシーンを保存しておく変数
	Scene* pendingNextScene = nullptr;

	AudioSource* Stage_BGM = nullptr;

	// 移動用タイマー（キャラクターのランダム移動用）
	std::vector<float> moveTimers;
};