#pragma once
#include <string>
#include <System/Sprite.h>
#include <System/Input.h>
#include "GameSprite.h"

class ChangeSceneSytem :public GameSprite
{
public:

	ChangeSceneSytem();

	~ChangeSceneSytem();

	// XVˆ—
	void Update(float elapsedTime)override;

	// •`‰æˆ—
	void Render()override;

	// GUI•`‰æ
	void DrawGUI();

private:

	static int stage_namber;


};