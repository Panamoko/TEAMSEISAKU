
#include "ChangeSceneSytem.h"
#include <System/Graphics.h>

#include "SceneManager.h"
#include "SceneLoading.h"
#include "SceneGame.h"
#include "SceneFactory.h"

ChangeSceneSytem::ChangeSceneSytem()
{



}

ChangeSceneSytem::~ChangeSceneSytem()
{
	delete sprite;
}

void ChangeSceneSytem::Update(float elapsedTime)
{
	Mouse& mouse = Input::Instance().GetMouse();

	if (mouse.GetButtonDown() && Mouse::BTN_LEFT)
	{
		SceneManager::Instance().ChangeScene(new SceneLoading(new SceneGame));
	}
}

void ChangeSceneSytem::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	// 描画準備
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;

	// 2Dスプライト描画
	{
		// タイトル（スプライト）描画
		float screenWidth = static_cast<float>(graphics.GetScreenWidth());
		float screenHeight = static_cast<float>(graphics.GetScreenHeight());
		sprite->Render(rc,				//&rc
			0, 0, 0,					//dx , dy , dz
			screenWidth, screenHeight,	//dw , dh
			0,							//angle
			1, 1, 1, 1);				//color
	}
}

void ChangeSceneSytem::DrawGUI()
{
}
