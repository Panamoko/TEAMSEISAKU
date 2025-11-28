#pragma once
#include "System/Sprite.h"
#include "Scene.h"
#include "Editor.h"

class SceneSelect :public Scene
{
public:
	void Initialize()override;
	void Update(float elapsedTime)override;
	void Render() override;
	void DrawGUI() override;


private:
	std::vector<std::unique_ptr<GameSprite>> sprites;
};

