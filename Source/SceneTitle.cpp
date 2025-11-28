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
	scene_name = "scene_title";
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
	}
}

// GUI描画
void SceneTitle::DrawGUI()
{
}

REGISTER_SCENE(SceneTitle);

