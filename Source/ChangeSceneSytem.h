#pragma once
#include <string>
#include <System/Sprite.h>
#include <System/Input.h>

class ChangeSceneSytem
{
public:

	ChangeSceneSytem();

	~ChangeSceneSytem();

	// XVˆ—
	void Update(float elapsedTime);

	// •`‰æˆ—
	void Render();

	// GUI•`‰æ
	void DrawGUI();

private:
	Sprite* sprite = nullptr;


};