#pragma once

#include "System/Sprite.h"
#include "Scene.h"
#include "Editor.h"
#include "ChangeSceneSytem.h"
#include "Dissolve.h"

// タイトルシーン
class SceneTitle : public Scene
{
public:
	SceneTitle() {}
	~SceneTitle() override {}

	void Initialize() override;
	void Finalize() override;
	void Update(float elapsedTime) override;
	void Render() override;
	void DrawGUI() override;

private:
	Sprite* sprite = nullptr;      // タイトルロゴ
	Sprite* spriteStart = nullptr; // Game Startボタン (GameStage.png)
	Sprite* spriteTutorial = nullptr; // ★追加: Tutorialボタン

	editor game_editor;
	std::vector<std::shared_ptr<GameObject>> objects;
	std::vector<std::unique_ptr<GameSprite>> sprites;

	// ボタンの座標とサイズ
	DirectX::XMFLOAT3 startButtonPos;
	DirectX::XMFLOAT3 startButtonSize;
	DirectX::XMFLOAT3 tutorialButtonPos;
	DirectX::XMFLOAT3 tutorialButtonSize;

	float alpha_timer = 0.0f;
	float blink_interval = 0.5f;
	DirectX::XMFLOAT4 render_color = { 1.0f, 1.0f, 1.0f, 1.0f };
	std::unique_ptr<ChangeSceneSytem> change_scene;

	// ディゾルブ制御用
	std::unique_ptr<Dissolve> dissolve;
	bool isSceneChanging = false;   // 遷移開始フラグ
	float transitionTimer = 0.0f;   // タイマー
	const float transitionDuration = 1.5f; // 暗転にかかる時間

	// ★追加: 遷移先のシーンを保存しておく変数
	Scene* pendingNextScene = nullptr;
};