#include "GameSprite.h"
#include <System/Graphics.h>

GameSprite::GameSprite()
{
	name = "Enpty";
	position = { 0.0f,0.0f };
	size = { 0.0f,0.0f };
	uv_max = { 0.0f,0.0f };
	uv_min = { 0.0f,0.0f };
	rotation = 0.0f;
	sprite_index = 0;
	color = { 1.0f,1.0f,1.0f,1.0f };
}

void GameSprite::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	// ï`âÊèÄîı
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;

	sprite.Render(
		rc,
		position.x, position.y, 0.0f,
		size.x, size.y,
		uv_min.x, uv_min.y,
		uv_max.x, uv_max.y,
		rotation,
		color.x, color.y, color.z, color.w
	);

}
