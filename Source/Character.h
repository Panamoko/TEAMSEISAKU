#pragma once

#include <DirectXMath.h>
#include "System/ShapeRenderer.h"
#include "ModelManager.h"
#include "Editor.h"
#include "GameObject.h"

//キャラクター
class Character:public GameObject
{
public:
	Character() {};
	virtual ~Character() {};

	//行列更新処理
	void UpdateTransform();

	// 位置取得
	const DirectX::XMFLOAT3& GetPosition() const { return position; }

	// 位置設定
	void SetPosition(const DirectX::XMFLOAT3& position) { this->position = position; }

	// 回転取得
	const DirectX::XMFLOAT3& GetAngle() const { return angle; }

	// 回転設定
	void SetAngle(const DirectX::XMFLOAT3& angle) { this->angle = angle; }

	// スケール取得
	const DirectX::XMFLOAT3& GetScale() const { return scale; }

	// スケール取得
	void SetScale(const DirectX::XMFLOAT3& scale) { this->scale = scale; }

	//半径取得
	float GetRadius() const { return radius; }

	//デバッグプリミティブ描画
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer);

	// 地面に接地しているか
	bool IsGround() const { return isGround; }

	// 高さ取得
	float GetHeight() const { return height; }

	// HP取得
	int GetHealth() const { return health; }
	
	// 最大HP取得（現在は初期値5で固定のようなので、適宜定数か変数を返します）
	int GetMaxHealth() const { return 5; }

	// 衝突処理
	void OnCollision(GameObject* object) override;

	// ダメージを与える
	bool ApplyDamage(int damage, float invincibleTime);

	// 回復させる
	void Heal(int amount);

	// 衝撃を与える
	void AddImpulse(const DirectX::XMFLOAT3& impulse);

protected:
	// 移動処理
	void Move(float elapsedTime, float vx, float vz, float speed);

	// 旋回処理
	void Turn(float elapsedTime, float vx, float vz, float speed);

	// ジャンプ処理
	void Jump(float speed);

	//速力処理更新
	void UpdateVelocity(float elapsedTime);

	//着地した時に呼ばれる
	virtual void OnLanding() {}

	// ダメージを受けた時に呼ばれる
	virtual void OnDamaged() {}

	// 死亡した時に呼ばれる
	virtual void OnDead() {}

	// 無敵時間更新
	void UpdateInvincibleTimer(float elapsedTime);

	void set_position(DirectX::XMFLOAT3 pos) { position = pos; };
	void set_rotation(DirectX::XMFLOAT3 rot) { angle = rot; };
	void set_scale(DirectX::XMFLOAT3 sca) { scale = sca; };

private:
	// 垂直速力更新処理
	void UpdateVerticalVelocity(float elapsedFrame);

	// 垂直移動更新処理
	void UpdateVerticalMove(float elapsedTime);

	// 水平速力更新処理
	void UpdateHorizontalVelocity(float elapsedFrame);

	// 水平移動更新処理
	void UpdateHorizontalMove(float elapsedTime);


protected:
	//DirectX::XMFLOAT3	position = {0,0,0};
	//DirectX::XMFLOAT3	angle = {0,0,0};
	//DirectX::XMFLOAT3	scale = {1,1,1};
	//DirectX::XMFLOAT4X4	transform = {
	//	1,0,0,0,
	//	0,1,0,0,
	//	0,0,1,0,
	//	0,0,0,1
	//};

	std::vector<std::unique_ptr<GameObject>> objects;

	float radius = 0.5f;
	float gravity = -30.0f;
	DirectX::XMFLOAT3 velocity = { 0,0,0 };
	bool isGround = false;
	float					height = 2.0f;
	int						health = 5;
	float					invincibleTimer = 1.0f;
	float					friction = 15.0f;
	float					acceleration = 50.0f;
	float					maxMoveSpeed = 5.0f;
	float					moveVecX = 0.0f;
	float					moveVecZ = 0.0f;

	float					airControl = 0.3f;
};


