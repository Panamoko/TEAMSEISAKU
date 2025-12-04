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

	BUTTON_WIDTH = 200.0f;
	BUTTON_HEIGHT = 100.0f;
	BUTTON_Y = 600.0f;
	COLOR_R = 1.0f, COLOR_G = 1.0f, COLOR_B = 1.0f, COLOR_A = 1.0f;
	BACK_BUTTON_X = 300.0f;
	NEXT_BUTTON_X = 600.0f;
	next_image_coror_a = 0.7f;
	back_image_coror_a = 0.7f;

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
	current_index = 1;
	end_sprite_namber = false;

	click_sprite[0] = SpriteManager::Instance().Load("Data/Sprite/next_image.png");
	click_sprite[1] = SpriteManager::Instance().Load("Data/Sprite/back_image.png");
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

	// 毎フレーム、透明度をデフォルト値にリセット
	next_image_coror_a = 0.7f;
	back_image_coror_a = 0.7f;

	// --- ボタンの当たり判定とクリック処理 ---

	// ボタンのサイズとY座標 (Renderで使用しているメンバー変数を利用)
	const DirectX::XMFLOAT2 button_size = { BUTTON_WIDTH, BUTTON_HEIGHT };
	const DirectX::XMFLOAT2 next_button_pos = { NEXT_BUTTON_X, BUTTON_Y };
	const DirectX::XMFLOAT2 back_button_pos = { BACK_BUTTON_X, BUTTON_Y };

	// 1. 「次へ」ボタンの判定 (最後のページの手前まで表示)
	if (current_index >= 0 && current_index < sprite.size() - 1)
	{
		// 当たり判定公式：点(mouse_pos)と四角(next_button_pos, button_size)
		if (Collision::IntersectPosSquare(mouse_pos, next_button_pos, button_size))
		{
			next_image_coror_a = 1.0f; // マウスオーバーで透明度を 1.0f に

			// クリック判定
			if (mouse.GetButtonDown() && Mouse::BTN_LEFT)
			{
				current_index++; // ページを進める
			}
		}
		else
		{
			next_image_coror_a = 0.7f;
		}
	}

	// 2. 「戻る」ボタンの判定 (最初のページではない場合のみ表示)
	if (current_index > 0 && current_index < sprite.size())
	{
		if (Collision::IntersectPosSquare(mouse_pos, back_button_pos, button_size))
		{
			back_image_coror_a = 1.0f; // マウスオーバーで透明度を 1.0f に

			// クリック判定
			if (mouse.GetButtonDown() && Mouse::BTN_LEFT)
			{
				current_index--; // ページを戻す
				end_sprite_namber = false; // 戻ったので終了フラグは false
			}
		}
		else
		{
			back_image_coror_a = 0.7f;
		}
	}

	// 3. ページ数の最終チェック
	// 次のページに進んで最終ページを超えた場合の処理
	if (current_index >= sprite.size())
	{
		current_index = -1; // -1 にして描画を停止
		end_sprite_namber = true;
	}
}

void TutorialSprite::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	// 描画準備
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

	if (current_index >= 0 && current_index < sprite.size() && click_sprite.size() >= 2) 
	{
		// 描画位置、サイズ、色を直接指定して描画
		if (current_index >= 0 && current_index < sprite.size() && click_sprite.size() >= 2)
		{
			// 描画パラメータ（ボタン共通）

			// **A. 「次へ」ボタン (click_sprite[0]) の描画**
			if (click_sprite[0] != nullptr)
			{
				const float NEXT_BUTTON_X = 600.0f; // 例として X座標を指定
				click_sprite[0]->Render(
					rc,
					NEXT_BUTTON_X, BUTTON_Y, 0.0f, // 描画位置
					BUTTON_WIDTH, BUTTON_HEIGHT,   // 描画サイズ
					0.0f,                          // 回転角度
					COLOR_R, COLOR_G, COLOR_B, next_image_coror_a // 描画色
				);
			}

			// **B. 「戻る」ボタン (click_sprite[1]) の描画**
			// 最初のチュートリアル画面 (current_index == 0) ではない場合のみ描画する
			if (current_index > 0 && click_sprite[1] != nullptr)
			{
				click_sprite[1]->Render(
					rc,
					BACK_BUTTON_X, BUTTON_Y, 0.0f, // 描画位置
					BUTTON_WIDTH, BUTTON_HEIGHT,   // 描画サイズ
					0.0f,                          // 回転角度
					COLOR_R, COLOR_G, COLOR_B, back_image_coror_a // 描画色
				);
			}
		}
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
			sprite.clear(); // 失敗した場合は中途半端なリストをクリア
			return false;
		}

		sprite.push_back(std::move(*unique_sprite));
	}

	return true;
}
