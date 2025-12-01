#include "Cannon.h"
#include "Factory.h"
#include "CollisionManager.h"
#include "ProjectileStraite.h"

Cannon::Cannon()
{
    class_name = "Cannon";

    //基礎設定
	hp = 50.0f;
	attac_interval = 3.0f;
	attac_territory = 10.0f;
	attac_timer = 0.0f;
	power = 10.0f;
	speed = 3.0f;

    //当たり判定の種類設定
    collider = std::make_unique<CylinderCollider>();
    collider->type = ColliderType::Cylinder;
    collider->owner = this;
    cylinder = static_cast<CylinderCollider*>(collider.get());
    CollisionManager::Instance().AddObject(this);
    cylinder->height = 5.0f;
    cylinder->radius = 3.0f;
}

void Cannon::Update(float elapsedTime)
{
    if (hp <= 0.0f)
    {
        GimmicManager::Instance().Remove(this);
        CollisionManager::Instance().Remove(this);
        return;
    }

    cylinder->center = position;
	attac_timer += elapsedTime;
    if (attac_timer >= attac_interval)attac_timer = attac_interval;

    DirectX::XMVECTOR cannon_pos = DirectX::XMLoadFloat3(&position);
    DirectX::XMVECTOR player_pos = DirectX::XMLoadFloat3(&player.GetPosition());

    //距離ベクトルを計算
    DirectX::XMVECTOR distance_vec = DirectX::XMVectorSubtract(player_pos, cannon_pos);

    //ベクトルの長さを計算
    float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(distance_vec));

    if (distance <= attac_territory)
    {
        Turn(elapsedTime, player.GetPosition());

        if (attac_timer >= attac_interval)
        {
            //大砲の現在の向き（発射方向）を計算

            //角度を XMVECTOR にロード
            DirectX::XMVECTOR rotation_quaternion = DirectX::XMQuaternionRotationRollPitchYaw(angle.x, angle.y, angle.z);
            
            //順方向 (Z軸) の単位ベクトル
            DirectX::XMVECTOR forward_vector = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

            //順方向ベクトルを回転させる
            DirectX::XMVECTOR launch_direction_vec = DirectX::XMVector3Rotate(forward_vector, rotation_quaternion);

            //XMFLOAT3 に格納
            DirectX::XMFLOAT3 launch_direction;
            DirectX::XMStoreFloat3(&launch_direction, launch_direction_vec);

            //ProjectileStraiteのインスタンスを生成
            ProjectileStraite* stratite = new ProjectileStraite(&projectileManager);

            stratite->Launch(launch_direction, position);

            attac_timer = 0.0f;
        }
    }

    projectileManager.Update(elapsedTime);


}

void Cannon::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    renderer->Render(rc, transform, model, ShaderId::Lambert);

    //弾丸描画処理
    projectileManager.Render(rc, renderer);
}

void Cannon::Turn(float elapsedTime, DirectX::XMFLOAT3 player_position)
{
	//大砲の位置とプレイヤーの位置を取得
	DirectX::XMVECTOR cannon_pos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR player_pos = DirectX::XMLoadFloat3(&player.GetPosition());

	//プレイヤーへの方向ベクトルを計算
	DirectX::XMVECTOR direction_to_player = DirectX::XMVectorSubtract(player_pos, cannon_pos);

	//方向ベクトルを正規化
	direction_to_player = DirectX::XMVector3Normalize(direction_to_player);

	//目標角度の計算
	float direction_x = DirectX::XMVectorGetX(direction_to_player);
	float direction_y = DirectX::XMVectorGetY(direction_to_player);
	float direction_z = DirectX::XMVectorGetZ(direction_to_player);

    //目標 X 軸回転（Pitch: 垂直方向の回転）の計算
    //水平方向の距離 (X-Z平面の長さ)
    float horizontal_distance = std::sqrt(direction_x * direction_x + direction_z * direction_z);

    //Y成分（高さ）と水平距離から Pitch（垂直角度）を計算
    float target_pitch = std::atan2(direction_y, horizontal_distance);

    //目標 Z 軸回転（Yaw: 水平方向の回転）の計算
    //Z軸（前方）と X軸（横）から水平角度を計算
    //angle.z を水平回転 (Yaw) に割り当てます
    float target_yaw_z = std::atan2(direction_x, direction_z);

    //回転処理 (X軸とZ軸のみ)
    float max_rotation = speed * elapsedTime;

    //Z軸回転 (angle.z - 水平方向) の回転処理

    float current_yaw_z = angle.z;
    float yaw_z_difference = target_yaw_z - current_yaw_z;

    //角度差を -PI から PI の間に収める (最短経路で回転)
    while (yaw_z_difference > DirectX::XM_PI)
    {
        yaw_z_difference -= DirectX::XM_2PI;
    }
    while (yaw_z_difference < -DirectX::XM_PI)
    {
        yaw_z_difference += DirectX::XM_2PI;
    }

    //実際に回転させる角度を決定 (回転量)
    float yaw_z_rotation_amount;
    if (std::abs(yaw_z_difference) > max_rotation)
    {
        yaw_z_rotation_amount = (yaw_z_difference > 0) ? max_rotation : -max_rotation;
    }
    else
    {
        yaw_z_rotation_amount = yaw_z_difference;
    }

    //Z軸の角度を更新
    angle.z += yaw_z_rotation_amount;

    //Z軸の角度を -PI から PI の間にクランプ
    while (angle.z > DirectX::XM_PI)
    {
        angle.z -= DirectX::XM_2PI;
    }
    while (angle.z < -DirectX::XM_PI)
    {
        angle.z += DirectX::XM_2PI;
    }

    //X 軸回転 (angle.x - 垂直方向) の回転処理

    float current_pitch = angle.x;
    float pitch_difference = target_pitch - current_pitch;

    //角度差を -PI から PI の間に収める (最短経路で回転)
    while (pitch_difference > DirectX::XM_PI)
    {
        pitch_difference -= DirectX::XM_2PI;
    }
    while (pitch_difference < -DirectX::XM_PI)
    {
        pitch_difference += DirectX::XM_2PI;
    }

    //実際に回転させる角度を決定
    float pitch_rotation_amount;
    if (std::abs(pitch_difference) > max_rotation)
    {
        pitch_rotation_amount = (pitch_difference > 0) ? max_rotation : -max_rotation;
    }
    else
    {
        pitch_rotation_amount = pitch_difference;
    }

    //X軸の角度を更新
    angle.x += pitch_rotation_amount;

    //X軸の角度を -PI から PI の間にクランプ
    while (angle.x > DirectX::XM_PI)
    {
        angle.x -= DirectX::XM_2PI;
    }
    while (angle.x < -DirectX::XM_PI)
    {
        angle.x += DirectX::XM_2PI;
    }
}

void Cannon::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
}

void Cannon::OnCollision(GameObject* object)
{
    if (object->type == Type::PlayerAttack)
    {
        hp -= 20.0f;
    }
}

bool Cannon::OnImGui()
{
	return false;
}

void Cannon::CopyUniqueMembers(const GameObject* source)
{
}

REGISTER_GAMEOBJECT(Cannon);