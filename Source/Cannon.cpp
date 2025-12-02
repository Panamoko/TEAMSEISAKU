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

    UpdateInvicible(elapsedTime);

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

                float offset_distance = 4.0f;

                //大砲の位置と発射方向を XMVECTOR にロード
                DirectX::XMVECTOR current_position_vec = DirectX::XMLoadFloat3(&position);
                current_position_vec = DirectX::XMVector3Normalize(current_position_vec);

                //発射方向ベクトルにオフセット距離を掛ける
                DirectX::XMVECTOR offset_vector = DirectX::XMVectorScale(launch_direction_vec, offset_distance);

                //大砲の位置 + オフセットベクトル = 新しい発射位置
                DirectX::XMVECTOR launceh_position_vec = DirectX::XMVectorAdd(current_position_vec, offset_vector);

                //XMFLOAT3 に格納
                DirectX::XMFLOAT3 launch_position;
                DirectX::XMStoreFloat3(&launch_position, launceh_position_vec);

                launch_position = {
                    launch_position.x + position.x,
                    launch_position.y + position.y,
                    launch_position.z + position.z
                };

                //ProjectileStraiteのインスタンスを生成
                ProjectileStraite* stratite = new ProjectileStraite(&projectileManager);

                stratite->Launch(launch_direction, { launch_position.x,launch_position.y + 1.0f,launch_position.z });

                attac_timer = 0.0f;
            }
            break;
        }
    }

    projectileManager.Update(elapsedTime);


}

void Cannon::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    if (invincible_timer <= 0.0f)
    {
        renderer->Render(rc, transform, model, ShaderId::Lambert);
    }
    else
    {
        renderer->Render(rc, transform, model, ShaderId::Lambert, { 1.0f,0.0f,0.0f,1.0f });
    }

    //弾丸描画処理
    projectileManager.Render(rc, renderer);
}

void Cannon::Turn(float elapsedTime, float vx, float vz, float speed)
{
    speed *= elapsedTime;

    // 進行ベクトルがゼロベクトルの場合は処理する必要なし
    float length = sqrtf(vx * vx + vz * vz);
    if (length < 0.001f) return;

    // 進行ベクトルを単位ベクトル化
    vx /= length;
    vz /= length;

    // 自身の回転値から前方向を求める
    float frontX = sinf(angle.y);
    float frontZ = cosf(angle.y);

    //--- ガタつきに対応させる ---

        // 回転角を求めるため、２つの単位ベクトルの内積を計算する
    float dot = (frontX * vx) + (frontZ * vz);	//内積：フロントが基準

    // 内積値は-1.0～1.0で表現されており、２つの単位ベクトルの角度が
    // 小さいほど1.0に近づくという性質を利用して回転速度を調整する
    float rot = 1.0f - dot;	//補正値
    //rot = 1.0f - dot;	//ImGuiで表示するためにメンバー変数とした
    if (rot > speed) rot = speed;	//回転速度よりも、rotが大きい場合は、回転速度を使う

    // 左右判定を行うために２つの単位ベクトルの外積を計算する
    float cross = (frontZ * vx) - (frontX * vz);

    // 2Dの外積値が正の場合か負の場合によって左右判定が行える
    // 左右判定を行うことによって左右回転を選択する
    if (cross < 0.0f)
    {
        //angle.y -= speed;
        angle.y -= rot;
    }
    else
    {
        //angle.y += speed;
        angle.y += rot;
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
    if (object->type == Type::PlayerAttack && invincible_timer <= 0.0f)
    {
        hp -= 20.0f;
        invincible_timer = 0.1f;
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