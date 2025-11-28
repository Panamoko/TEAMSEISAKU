#include "System/Graphics.h"
#include "SceneTitle.h"

#include "System/Input.h"
#include "SceneGame.h"
#include "SceneManager.h"
#include "SceneLoading.h"
#include "SceneFactory.h"
#include "SpriteManager.h"

// 初期化
void SceneTitle::Initialize()
{
	// スプライト初期化
	sprite = SpriteManager::Instance().Load("Data/Sprite/Title.png");
	sprite2 = SpriteManager::Instance().Load("Data/Sprite/GameStage.png");

	scene_name = "scene_title";
	position = { 430,490,0 };
	size = { 500,300,0 };
	sprite_left = position.x;
	sprite_top = position.y;
	sprite_width = size.x;
	sprite_height = size.y;
}

// 終了化
void SceneTitle::Finalize()
{
	// スプライト終了化
	//if (sprite != nullptr)
	//{
	//	delete sprite;
	//	sprite = nullptr;
	//}
}

// 更新処理
void SceneTitle::Update(float elapsedTime)
{
	alpha_timer += elapsedTime;

	if (std::fmod(alpha_timer, blink_interval * 2.0f) < blink_interval)
	{
		// ONの状態（不透明）
		render_color.w = 1.0f;
	}
	else
	{
		// OFFの状態（透明、または半透明）
		render_color.w = 0.0f; // 完全に透明にして見えなくする
		// 半透明でチカチカさせたい場合は 0.5f などに設定
	}

	GamePad& gamePad = Input::Instance().GetGamePad();

	// なにかボタンを押したらローディングシーンへ切り替え
	const GamePadButton anyButton =
		GamePad::BTN_A
		| GamePad::BTN_B
		| GamePad::BTN_X
		| GamePad::BTN_Y
		;
	if (gamePad.GetButtonDown() & anyButton)
	{
		//SceneManager::Instance().ChangeScene(new SceneGame);
		SceneManager::Instance().ChangeScene(new SceneLoading(new SceneGame));
	}

	Mouse& mouse = Input::Instance().GetMouse();
	DirectX::XMFLOAT2 mouse_position;

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

//描画処理
void SceneTitle::Render()
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	RenderState* renderState = graphics.GetRenderState();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	game_editor.render(objects, sprites, ModelManager::Instance().GetModels(), modelRenderer);

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

		sprite2->Render(rc,				//&rc
			position.x, position.y, position.z,					//dx , dy , dz
			size.x, size.y,	//dw , dh
			0,							//angle
			render_color.x, render_color.y, render_color.z, render_color.w);				//color

	}
}

// GUI描画
void SceneTitle::DrawGUI()
{
}

REGISTER_SCENE(SceneTitle);

