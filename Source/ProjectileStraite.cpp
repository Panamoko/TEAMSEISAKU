#include "ProjectileStraite.h"
#include "Collision.h"

//ProjectileStraite::ProjectileStraite()
//{
//	model = new Model("Data/Model/SpikeBall/SpikeBall.mdl");
//
//	// 表示サイズを調整
//	scale.x = scale.y = scale.z = 0.5f;
//}

ProjectileStraite::ProjectileStraite(ProjectileManager* manager)
	:Projectile(manager)	//基底クラスのコンストラクタうぃ呼び出す
{
	//model = new Model("Data/Model/SpikeBall/SpikeBall.mdl");
	model = new Model("Data/Model/Sword/Sword.mdl");

	// 表示サイズを調整
//	scale.x = scale.y = scale.z = 0.5f;
	scale.x = scale.y = scale.z = 3.0f;
}

// デストラクタ
ProjectileStraite::~ProjectileStraite()
{
	delete model;
}

// 更新処理
void ProjectileStraite::Update(float elapsedTime)
{
	//寿命
	lifeTimer -= elapsedTime;
	if (lifeTimer <= 0.0f)
	{
		//自分を削除
		Destroy();//寿命が尽きたら、自分を破棄する。
	}
	// 移動
	float speed = this->speed * elapsedTime;
	position.x += direction.x * speed;
	position.y += direction.y * speed;
	position.z += direction.z * speed;

	// オブジェクト行列を更新
	UpdateTransform();

	// モデル行列更新
	model->UpdateTransform();
}

void ProjectileStraite::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Render(rc, transform, model, ShaderId::Lambert);

}

// 発射
void ProjectileStraite::Launch(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& position)
{
	this->direction = direction;
	this->position = position;
}
