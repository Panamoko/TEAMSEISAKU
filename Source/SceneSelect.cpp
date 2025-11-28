#include "SceneSelect.h"
#include <System/GamePad.h>
#include "System/Input.h"

void SceneSelect::Initialize()
{
}

void SceneSelect::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	for (auto& sprite : sprites)
	{
		if (sprite)
		{
			sprite->Update(elapsedTime);
		}
	}

}

void SceneSelect::Render()
{
	for (auto& sprite : sprites)
	{
		if (sprite)
		{
			sprite->Render();
		}
	}

}

void SceneSelect::DrawGUI()
{
}
