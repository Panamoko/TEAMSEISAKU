#include "TutorialSprite.h"
#include "Collision.h"
#include <System/Graphics.h>
#include <System/Mouse.h>
#include <System/Input.h>

TutorialSprite::TutorialSprite()
{
	click_pos = { 0.0f,0.0f };
	click_size = { 1280.0f,720.0f };
	position = { 0.0f,0.0f };
	color = { 1.0f,1.0f,1.0f,1.0f };
	sprite_paths = {
		"Data/Sprite/tutorial_sprite_01.png",
		"Data/Sprite/tutorial_sprite_02.png",
		"Data/Sprite/tutorial_sprite_03.png",
		"Data/Sprite/tutorial_sprite_04.png",
		"Data/Sprite/tutorial_sprite_05.png"
	};

	if (!LoadSprite(sprite_paths))
	{
		std::cerr << "Fatal Error: Tutorial images could not be loaded." << std::endl;
	}
}

void TutorialSprite::AddSprite(const Sprite& sprite_date)
{
	sprite.push_back(sprite_date);
}

void TutorialSprite::Update()
{
	Mouse& mouse = Input::Instance().GetMouse();

	mouse_pos.x = static_cast<float>(mouse.GetPositionX());
	mouse_pos.y = static_cast<float>(mouse.GetPositionY());

	bool is_mouse_over_sprite = Collision::IntersectPosSquare(
		mouse_pos,
		click_pos,
		click_size);

	if (mouse.GetButtonDown() && Mouse::BTN_LEFT && is_mouse_over_sprite)
	{
		current_index++;

		if (current_index >= sprite.size())
		{
			current_index = -1;

		}
	}

}

void TutorialSprite::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	// •`‰æ€”õ
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;

	if (current_index >= 0 && current_index < sprite.size())
	{
		sprite[current_index].Render(
			rc,
			position.x, position.y, 0,					//dx , dy , dz
			click_size.x, click_size.y,	//dw , dh
			0,							//angle
			color.x, color.y, color.z, color.w);				//color
	}
}

bool TutorialSprite::LoadSprite(const std::vector<std::string>& file_paths)
{
	sprite.clear();

	SpriteManager& manager = SpriteManager::Instance();

	for (const std::string& path : file_paths)
	{
		auto unique_sprite = manager.CreateUniqueInstance(path);
		if (!unique_sprite)
		{
			std::cerr << "Error: Failed to load tutorial image using SpriteManager: " << path << std::endl;
			sprite.clear(); // Ž¸”s‚µ‚½ê‡‚Í’†“r”¼’[‚ÈƒŠƒXƒg‚ðƒNƒŠƒA
			return false;
		}

		sprite.push_back(std::move(*unique_sprite));
	}

	return true;
}
