
#include "ChangeSceneSytem.h"
#include <System/Graphics.h>

#include "SceneManager.h"
#include "SceneLoading.h"
#include "SceneGame.h"
#include "SceneFactory.h"

int ChangeSceneSytem::stage_namber = 0;


ChangeSceneSytem::ChangeSceneSytem()
{
	sprite_vector[0] = SpriteManager::Instance().Load("Data/Sprite/Tutorial.png");
	sprite_vector[1] = SpriteManager::Instance().Load("Data/Sprite/Stage01.png");
	position.x = 100.0f;
	position.y = 100.0f;
	size = { 256.0f,128.0f };
	sprite_width = size.x;
	sprite_height = size.y;
	float sprite_left = position.x;
	float sprite_top = position.y;
}

ChangeSceneSytem::~ChangeSceneSytem()
{
}

void ChangeSceneSytem::Update(float elapsedTime)
{
	Mouse& mouse = Input::Instance().GetMouse();

	mouse_position.x = static_cast<float>(mouse.GetPositionX());
	mouse_position.y = static_cast<float>(mouse.GetPositionY());

	bool is_x_inside = (mouse_position.x >= sprite_left) && (mouse_position.x < sprite_left + sprite_width);
	bool is_y_inside = (mouse_position.y >= sprite_top) && (mouse_position.y < sprite_top + sprite_height);

	bool is_mouse_over_sprite = is_x_inside && is_y_inside;

	if (mouse.GetButtonDown() && Mouse::BTN_LEFT && is_mouse_over_sprite)
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
