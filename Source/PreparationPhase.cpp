#include "PreparationPhase.h"
#include <System/Mouse.h>
#include <System/Input.h>
#include <Collision.h>
#include <System/Graphics.h>

PreparationPhase::PreparationPhase()
{
	state = false;
	sprite = SpriteManager::Instance().Load("Data/Sprite/Preparation_Phase.png");
	sprite2 = SpriteManager::Instance().Load("Data/Sprite/State.png");
	position = { 550,-10 };
	sprite2_pos = { 950,550 };
	size = { 300.0f,200.0f };
	sprite2_size = size;
	sprite2_color = { 1.0f,1.0f,1.0f,0.7f };
}

//a
void PreparationPhase::Update(float elapsedTime)
{
	Mouse& mouse = Input::Instance().GetMouse();

	mouse_position.x = static_cast<float>(mouse.GetPositionX());
	mouse_position.y = static_cast<float>(mouse.GetPositionY());

	bool is_mouse_over_sprite = Collision::IntersectPosSquare(
		mouse_position,
		{ sprite2_pos.x,sprite2_pos.y },
		{ size.x ,size.y });

	if (is_mouse_over_sprite)
	{
		sprite2_color.w = 1.0f;
	}
	else
	{
		sprite2_color.w = 0.7f;
	}

	if (mouse.GetButtonDown() && Mouse::BTN_LEFT && is_mouse_over_sprite)
	{
		state = true;
	}

}

void PreparationPhase::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	// •`‰æ€”õ
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	if (!state)
	{
		sprite->Render(rc,				//&rc
			position.x, position.y, 0,					//dx , dy , dz
			size.x, size.y,	//dw , dh
			0,							//angle
			color.x, color.y, color.z, color.w);				//color

		sprite2->Render(rc,				//&rc
			sprite2_pos.x, sprite2_pos.y, 0,					//dx , dy , dz
			sprite2_size.x, sprite2_size.y,	//dw , dh
			0,							//angle
			sprite2_color.x, sprite2_color.y, sprite2_color.z, sprite2_color.w);				//color
	}

}