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
	position = { 640,10 };
	sprite2_pos = { 1000,600 };
	size = { 10.0f,10.0f };
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
		{ 300 ,100 });

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

	sprite->Render(rc,				//&rc
		position.x, position.y, 0,					//dx , dy , dz
		size.x, size.y,	//dw , dh
		0,							//angle
		color.x, color.y, color.z, color.w);				//color

	sprite2->Render(rc,				//&rc
		sprite2_pos.x, sprite2_pos.y, 0,					//dx , dy , dz
		size.x, size.y,	//dw , dh
		0,							//angle
		color.x, color.y, color.z, color.w);				//color

}