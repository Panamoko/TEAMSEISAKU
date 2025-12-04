
#include "ChangeSceneSytem.h"
#include <System/Graphics.h>

#include "SceneManager.h"
#include "SceneLoading.h"
#include "SceneGame.h"
#include "SceneFactory.h"
#include "Collision.h"

int ChangeSceneSytem::stage_namber = 0;


ChangeSceneSytem::ChangeSceneSytem()
{
	sprite_vector[0] = SpriteManager::Instance().Load("Data/Sprite/Tutorial.png");
	sprite_vector[1] = SpriteManager::Instance().Load("Data/Sprite/Stage01.png");
	position.x = 400.0f;
	position.y = 250.0f;
	size = { 512.0f,700.0f };
	sprite_width = size.x;
	sprite_height = size.y;
	float sprite_left = position.x;
	float sprite_top = position.y;
	color = { 1.0f,1.0f,1.0f,1.0f };
}

ChangeSceneSytem::~ChangeSceneSytem()
{
}

void ChangeSceneSytem::Update(float elapsedTime)
{
	alpha_timer += elapsedTime;

	if (std::fmod(alpha_timer, blink_interval * 2.0f) < blink_interval)
	{
		// ONの状態（不透明）
		color.w = 1.0f;
	}
	else
	{
		// OFFの状態（透明、または半透明）
		color.w = 0.0f; // 完全に透明にして見えなくする
		// 半透明でチカチカさせたい場合は 0.5f などに設定
	}


	Mouse& mouse = Input::Instance().GetMouse();

	mouse_position.x = static_cast<float>(mouse.GetPositionX());
	mouse_position.y = static_cast<float>(mouse.GetPositionY());

	bool is_mouse_over_sprite = Collision::IntersectPosSquare(
		mouse_position,
		{ position.x+150,position.y+350},
		{ 300 ,100 });

	if (mouse.GetButtonDown() && Mouse::BTN_LEFT && is_mouse_over_sprite)
	{
		std::string scene_name = "scene_tutorial";

		// 直接 SceneGame へ遷移する
		SceneManager::Instance().ChangeScene(new SceneGame(scene_name));
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
		sprite_vector[0]->Render(rc,				//&rc
			position.x, position.y, 0,					//dx , dy , dz
			size.x, size.y,	//dw , dh
			0,							//angle
			color.x, color.y, color.z, color.w);				//color
	}
}

void ChangeSceneSytem::DrawGUI()
{
}
