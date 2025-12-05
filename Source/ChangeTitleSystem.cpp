#include "ChangeTitleSystem.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include <Collision.h>
#include <System/Graphics.h>

ChangeTitleSystem::ChangeTitleSystem()
{
	//sprite_vector[0] = SpriteManager::Instance().Load();
	position = { 540,400 };
	size = { 300.0f,100.0f };
	color = { 1.0f,1.0f,1.0f,0.7f };
}

void ChangeTitleSystem::Update(float elapsedTime)
{
	Mouse& mouse = Input::Instance().GetMouse();

	mouse_position.x = static_cast<float>(mouse.GetPositionX());
	mouse_position.y = static_cast<float>(mouse.GetPositionY());

	bool is_mouse_over_sprite = Collision::IntersectPosSquare(
		mouse_position,
		{ position.x,position.y },
		size);

	if (is_mouse_over_sprite)
	{
		color.w = 1.0f;
	}
	else
	{
		color.w = 0.7f;
	}

	if (mouse.GetButtonDown() && Mouse::BTN_LEFT && is_mouse_over_sprite)
	{
		std::string scene_name = "scene_tutorial";

		// 直接 SceneGame へ遷移する
		SceneManager::Instance().ChangeScene(new SceneTitle());
	}

}

void ChangeTitleSystem::Render()
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
		sprite_vector[0]->Render(rc,				//&rc
			position.x, position.y, 0,					//dx , dy , dz
			size.x, size.y,	//dw , dh
			0,							//angle
			color.x, color.y, color.z, color.w);				//color
	}

}

void ChangeTitleSystem::DrawGUI()
{
}
