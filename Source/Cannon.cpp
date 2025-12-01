#include "Cannon.h"
#include "Factory.h"
#include "CollisionManager.h"
#include "ProjectileStraite.h"
#include <imgui.h>

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
    // 味方がいなければ Player を検索
    const auto& allPlayers = Player::GetAllPlayers();
    for (const auto* current_player : allPlayers)
    {
        if (!current_player || current_player->GetHealth() <= 0) continue;

        player_pos = current_player->GetPosition();


        //距離ベクトルを計算
        DirectX::XMFLOAT3 distance_vec = {
            player_pos.x - position.x,
            player_pos.y - position.y,
            player_pos.z - position.z
        };

        //ベクトルの長さを計算
        float distance = (
            (distance_vec.x * distance_vec.x) +
            (distance_vec.y * distance_vec.y) +
            (distance_vec.z * distance_vec.z));

        if (distance <= (attac_territory * attac_territory))
        {
            Turn(elapsedTime, player_pos);

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

                stratite->Launch(launch_direction, { position.x + 3.0f,position.y,position.z + 3.0f });

                attac_timer = 0.0f;
            }
            break;
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
	DirectX::XMVECTOR player_pos = DirectX::XMLoadFloat3(&player_position);

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

    //Y軸回転 (angle.y - 水平方向 Yaw) の回転処理

    float current_yaw_y = angle.y;
    float yaw_y_difference = target_yaw_z - current_yaw_y;

    //角度差を -PI から PI の間に収める
    while (yaw_y_difference > XM_PI)
    {
        yaw_y_difference -= XM_2PI;
    }
    while (yaw_y_difference < -XM_PI)
    {
        yaw_y_difference += XM_2PI;
    }

    //実際に回転させる角度を決定
    float yaw_y_rotation_amount;
    if (std::abs(yaw_y_difference) > max_rotation)
    {
        yaw_y_rotation_amount = (yaw_y_difference > 0) ? max_rotation : -max_rotation;
    }
    else
    {
        yaw_y_rotation_amount = yaw_y_difference;
    }

    //Y軸の角度を更新
    angle.y += yaw_y_rotation_amount;

    //Y軸の角度を -PI から PI の間にクランプ
    while (angle.y > XM_PI)
    {
        angle.y -= XM_2PI;
    }
    while (angle.y < -XM_PI)
    {
        angle.y += XM_2PI;
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
    //縄張り範囲をデバッグ円柱描画
    renderer->RenderCylinder(
        rc,
        position,
        attac_territory,
        1.0f,
        DirectX::XMFLOAT4(0, 1, 0, 1));

    renderer->RenderCylinder(
        rc, position,
        cylinder->radius,
        cylinder->height,
        DirectX::XMFLOAT4(1, 0, 0, 1)
    );

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
    bool changed = false;

    if (ImGui::CollapsingHeader("Cannon"))
    {
        changed |= ImGui::DragFloat("HP", &hp, 0.1f, 0.0f, 1500.0f, "%.1f");
        changed |= ImGui::DragFloat("Power", &power, 0.1f, 0.0f, 1500.0f, "%.1f");
        changed |= ImGui::DragFloat("Attac_interval", &attac_interval, 0.1f, 0.1f, 60.0f, "%.1f sec");
        changed |= ImGui::DragFloat("Attac_timer", &attac_timer, 0.1f, 0.0f, attac_interval * 2.0f, "%.1f sec");
    }
    return changed;

}

void Cannon::CopyUniqueMembers(const GameObject* source)
{
}

REGISTER_GAMEOBJECT(Cannon);