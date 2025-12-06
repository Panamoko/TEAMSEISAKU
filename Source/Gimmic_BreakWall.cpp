#include "Gimmic_BreakWall.h"
#include "Factory.h"
#include "Player.h"
#include "Core.h"
#include <imgui.h>
#include <imstb_truetype.h>
#include <DirectXTex.h>
#include <DDS.h>
#include <algorithm> // これが必要です
#include "Gimmic_BreakWall.h"
#include "Factory.h"
#include "Player.h" // ★Playerクラスを使うので必須
#include "Core.h"

using namespace std;

Gimmic_BreakWall::Gimmic_BreakWall()
{
	class_name = "Gimmic_BreakWall";
	model = ModelManager::Instance().Load("Data/Model/bilud/saku.mdl");

    collider = std::make_unique<OBB>();
    collider->type = ColliderType::OBB;
    box = static_cast<OBB*>(collider.get());

    scale.x = 0.1f;
    scale.y = 0.05f;
    scale.z = 0.1f;

    maxHp = 2.0f; 
    hp = maxHp;
    isBroken = false;
    isRespawning = false;
    respawnTimer = 0.0f;
    fadeInTimer = 0.0f; 

    color.w = 1.0f;

    CollisionManager::Instance().AddObject(this);

}

//衝突結果
void Gimmic_BreakWall::OnCollision(GameObject* objects)
{
    if (isBroken || isRespawning) return;

    if (objects->type == Type::PlayerAttack && invincible_timer <= 0.0f)
    {
        hitSE[0]->Play(false);
        hp--;
        invincible_timer = 0.1f;
    }

    if (hp <= 0.0f)
    {
        // ★修正ポイント: 壁が壊れたら、全プレイヤーに経路再計算を命令する
        const auto& players = Player::GetAllPlayers();
        for (auto* player : players)
        {
            if (player)
            {
                player->RequestPathRecalculation();
            }
        }

        // --- 既存の破壊処理 ---
        // is_active = false; // 前回のアドバイス通り、ここは削除またはコメントアウト
        isBroken = true;
        isRespawning = false;
        respawnTimer = respawnTime;
    }
}

//更新処理
void Gimmic_BreakWall::Update(float elapsedTime)
{
    if (!collider || !model) return;

    UpdateInvicible(elapsedTime);

    if (isRespawning)
    {
        fadeInTimer += elapsedTime;

        // フェードインの進行度 (0.0 -> 1.0) を計算
        float alpha = std::min<float>(fadeInTimer / fadeInDuration, 1.0f);
        color.w = alpha; // アルファ値を更新

        // フェードインが完了したら
        if (fadeInTimer >= fadeInDuration)
        {
            isRespawning = false;
            fadeInTimer = 0.0f;
            color.w = 1.0f; // 完全に不透明に
            is_active = true;
        }
    }

    else if (isBroken)
    {
        respawnTimer -= elapsedTime;

        // タイマーが0以下になったら判定開始
        if (respawnTimer <= 0.0f)
        {
            bool canRespawn = true; // デフォルトは許可

            // コアの位置取得
            Core* core = Core::Instance();

            // 全プレイヤーリストを取得
            const std::vector<Player*>& allPlayers = Player::GetAllPlayers();

            if (core && !allPlayers.empty())
            {
                DirectX::XMFLOAT3 corePos = core->position;
                DirectX::XMFLOAT3 wallPos = this->position;

                // 距離計算用ラムダ式
                auto DistSqXZ = [](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
                    float dx = a.x - b.x;
                    float dz = a.z - b.z;
                    return dx * dx + dz * dz;
                    };

                // 壁とコアの距離（基準）
                float distCoreToWall = DistSqXZ(corePos, wallPos);

                // 全プレイヤーをチェック
                for (Player* player : allPlayers)
                {
                    if (!player) continue;

                    DirectX::XMFLOAT3 playerPos = player->position; // GetPosition()がない場合はposition直接参照
                    float distCoreToPlayer = DistSqXZ(corePos, playerPos);

                    // 「誰か一人でも」壁より内側（コアに近い）なら
                    if (distCoreToPlayer < distCoreToWall)
                    {
                        canRespawn = false; // リポップ不可
                        break; // 一人でも内側にいればこれ以上調べる必要はないのでループを抜ける
                    }
                }
            }

            // リポップ処理
            if (canRespawn)
            {
                isBroken = false;
                isRespawning = true;
                hp = maxHp;
                respawnTimer = 0.0f;
                fadeInTimer = 0.0f;
                color.w = 0.0f;



            }
            else
            {
                // リポップ条件を満たしていないので、次回フレームで再チェックさせる
                // タイマーを0のままにしておくと毎フレームチェックが走る
                respawnTimer = 0.0f;
            }
        }
    }

    if (isBroken || isRespawning)
    {
        // 壊れている、または復活中は、当たり判定を無効化する
        // OBBのサイズを0にし、場所を地下深くに飛ばすことで物理的に当たらないようにする
        box->half = { 0.0f, 0.0f, 0.0f };
        box->center = { 0.0f, -1000.0f, 0.0f };
    }
    else
    {
        // 通常時：モデルに合わせてOBBを更新
        DirectX::XMFLOAT3 center;
        DirectX::XMFLOAT3 half;
        DirectX::XMFLOAT3 axis[3];

        DirectX::XMFLOAT3 rotation = {
            DirectX::XMConvertToRadians(angle.x),
            DirectX::XMConvertToRadians(angle.y),
            DirectX::XMConvertToRadians(angle.z)
        };

        // モデルの OBB を取得
        if (model->GetModelOBB(
            model,
            position,
            rotation,
            scale,
            center,
            half,
            axis))
        {
            box->center = center;
            box->half = half;

            box->axis[0] = axis[0];
            box->axis[1] = axis[1];
            box->axis[2] = axis[2];

            if (model) {
                printf("OBB center = (%f,%f,%f)\n", center.x, center.y, center.z);
                printf("OBB half   = (%f,%f,%f)\n", half.x, half.y, half.z);
                printf("axis0 = (%f,%f,%f)\n", axis[0].x, axis[0].y, axis[0].z);
                printf("axis1 = (%f,%f,%f)\n", axis[1].x, axis[1].y, axis[1].z);
                printf("axis2 = (%f,%f,%f)\n", axis[2].x, axis[2].y, axis[2].z);
            }
        }
    }
}

void Gimmic_BreakWall::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    if (isBroken) return;

    if (invincible_timer <= 0.0f)
    {
        GameObject::Render(rc, renderer);
    }
    else
    {
        renderer->Render(rc, transform, model, ShaderId::Lambert, { 1.0f,0.0f,0.0f,1.0f });
    }
}


void Gimmic_BreakWall::RenderDebugPrimitive(
    const RenderContext& rc, ShapeRenderer* renderer)
{
    if (isBroken) return;

    if (!renderer || !collider) return;

    DirectX::XMFLOAT3 obb_center = box->center;
    DirectX::XMFLOAT3 obb_half = box->half;
    DirectX::XMFLOAT3 obb_axis[3] = { box->axis[0], box->axis[1], box->axis[2] };
    DirectX::XMFLOAT4 debug_color = { 0.2f, 0.8f, 0.2f, 1.0f };

    renderer->RenderOBB(
        rc,
        obb_center,
        obb_half,
        obb_axis, // 3つの軸配列を渡す
        debug_color
    );
}

bool Gimmic_BreakWall::OnImGui()
{
    if (ImGui::CollapsingHeader("BreakWall Settings"))
    {
        ImGui::Checkbox("isBroken", &isBroken);
        ImGui::DragFloat("HP", &hp, 0.1f, 0.0f, maxHp, "%.1f");
        ImGui::DragFloat("Max HP", &maxHp, 0.1f, 1.0f, 1500.0f, "%.1f");
        ImGui::DragFloat("Respawn Time", &respawnTime, 0.1f, 1.0f, 60.0f, "%.1f");
        if (isBroken) {
            ImGui::Text("Respawning in: %.1f", respawnTimer);
            ImGui::ProgressBar(1.0f - (respawnTimer / respawnTime));
        }

        ImGui::DragFloat("HP", &hp, 0.1f, 0.0f, 1500.0f, "%.1f");
        ImGui::DragFloat3("size", &size.x, 0.1f, 0.0f, 1500.0f, "%.1f");

        ImGui::DragFloat3("center", &box->center.x, 0.1f, 0.0f, 100.0f, "%.1f");
        ImGui::DragFloat3("hal", &box->half.x, 0.1f, 0.0f, 100.0f, "%.1f");
        ImGui::DragFloat3("axis 1", &box->axis[0].x, 0.1f, 0.0f, 100.0f, "%.1f");
        ImGui::DragFloat3("axis 2", &box->axis[1].x, 0.1f, 0.0f, 100.0f, "%.1f");
        ImGui::DragFloat3("axis 3", &box->axis[2].x, 0.1f, 0.0f, 100.0f, "%.1f");

        return true;
    }
    return false;
}


REGISTER_GAMEOBJECT(Gimmic_BreakWall);

