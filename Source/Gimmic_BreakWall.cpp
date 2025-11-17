#include "Gimmic_BreakWall.h"
#include "Factory.h"
#include "Player.h"
#include "Core.h"
#include <imgui.h>
#include <imstb_truetype.h>
#include <DirectXTex.h>
#include <DDS.h>
#include <algorithm> // Ç±ÇÍÇ™ïKóvÇ≈Ç∑

using namespace std;
//a

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
<<<<<<< HEAD
=======
    damage = false;
>>>>>>> parent of ad9ee60 („ÉÄ„É°„Éº„Ç∏„ÇíÂèó„Åë„Åü„ÇâÊüµ„ÅåËµ§Ëâ≤„Å´„Å™„Çã)

    CollisionManager::Instance().AddObject(this);
}

//è’ìÀåãâ 
void Gimmic_BreakWall::OnCollision(GameObject* objects)
{
    if (isBroken || isRespawning) return;

    if (objects->type == Type::PlayerAttack)
<<<<<<< HEAD
=======
    {
>>>>>>> parent of ad9ee60 („ÉÄ„É°„Éº„Ç∏„ÇíÂèó„Åë„Åü„ÇâÊüµ„ÅåËµ§Ëâ≤„Å´„Å™„Çã)
        hp--;
    if (hp <= 0.0f)
    {
        is_active = false;
        isBroken = true;
        isRespawning = false;
        respawnTimer = respawnTime;
        CollisionManager::Instance().Remove(this);
    }
}

//çXêVèàóù
void Gimmic_BreakWall::Update(float elapsedTime)
{
    if (!collider || !model) return;

    if (isRespawning)
    {
        fadeInTimer += elapsedTime;

        // ÉtÉFÅ[ÉhÉCÉìÇÃêiçsìx (0.0 -> 1.0) ÇåvéZ
        float alpha = std::min<float>(fadeInTimer / fadeInDuration, 1.0f);
        color.w = alpha; // ÉAÉãÉtÉ@ílÇçXêV

        // ÉtÉFÅ[ÉhÉCÉìÇ™äÆóπÇµÇΩÇÁ
        if (fadeInTimer >= fadeInDuration)
        {
            isRespawning = false;
            fadeInTimer = 0.0f;
            color.w = 1.0f; // äÆëSÇ…ïsìßñæÇ…

            // Ç±Ç±Ç≈ìñÇΩÇËîªíËÇóLå¯âªÇ∑ÇÈ
            CollisionManager::Instance().AddObject(this);
            is_active = true;
        }

        DirectX::XMFLOAT3 center;
        DirectX::XMFLOAT3 half;
        DirectX::XMFLOAT3 axis[3];

        DirectX::XMFLOAT3 rotation = {
            DirectX::XMConvertToRadians(angle.x),
            DirectX::XMConvertToRadians(angle.y),
            DirectX::XMConvertToRadians(angle.z)
        };

        // ÉÇÉfÉãÇÃ OBB ÇéÊìæ
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

        return;
    }

    if (isBroken)
    {
        respawnTimer -= elapsedTime;

        // É^ÉCÉ}Å[Ç™0à»â∫Ç…Ç»Ç¡ÇΩÇÁîªíËäJén
        if (respawnTimer <= 0.0f)
        {
            bool canRespawn = true; // ÉfÉtÉHÉãÉgÇÕãñâ¬

            // ÉRÉAÇÃà íuéÊìæ
            Core* core = Core::Instance();

            // ëSÉvÉåÉCÉÑÅ[ÉäÉXÉgÇéÊìæ
            const std::vector<Player*>& allPlayers = Player::GetAllPlayers();

            if (core && !allPlayers.empty())
            {
                DirectX::XMFLOAT3 corePos = core->position;
                DirectX::XMFLOAT3 wallPos = this->position;

                // ãóó£åvéZópÉâÉÄÉ_éÆ
                auto DistSqXZ = [](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
                    float dx = a.x - b.x;
                    float dz = a.z - b.z;
                    return dx * dx + dz * dz;
                    };

                // ï«Ç∆ÉRÉAÇÃãóó£ÅiäÓèÄÅj
                float distCoreToWall = DistSqXZ(corePos, wallPos);

                // ëSÉvÉåÉCÉÑÅ[ÇÉ`ÉFÉbÉN
                for (Player* player : allPlayers)
                {
                    if (!player) continue;

                    DirectX::XMFLOAT3 playerPos = player->position; // GetPosition()Ç™Ç»Ç¢èÍçáÇÕpositioníºê⁄éQè∆
                    float distCoreToPlayer = DistSqXZ(corePos, playerPos);

                    // ÅuíNÇ©àÍêlÇ≈Ç‡Åvï«ÇÊÇËì‡ë§ÅiÉRÉAÇ…ãﬂÇ¢ÅjÇ»ÇÁ
                    if (distCoreToPlayer < distCoreToWall)
                    {
                        canRespawn = false; // ÉäÉ|ÉbÉvïsâ¬
                        break; // àÍêlÇ≈Ç‡ì‡ë§Ç…Ç¢ÇÍÇŒÇ±ÇÍà»è„í≤Ç◊ÇÈïKóvÇÕÇ»Ç¢ÇÃÇ≈ÉãÅ[ÉvÇî≤ÇØÇÈ
                    }
                }
            }

            // ÉäÉ|ÉbÉvèàóù
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
                // ÉäÉ|ÉbÉvèåèÇñûÇΩÇµÇƒÇ¢Ç»Ç¢ÇÃÇ≈ÅAéüâÒÉtÉåÅ[ÉÄÇ≈çƒÉ`ÉFÉbÉNÇ≥ÇπÇÈ
                // É^ÉCÉ}Å[Ç0ÇÃÇ‹Ç‹Ç…ÇµÇƒÇ®Ç≠Ç∆ñàÉtÉåÅ[ÉÄÉ`ÉFÉbÉNÇ™ëñÇÈ
                respawnTimer = 0.0f;
            }
        }
        return;
    }

    DirectX::XMFLOAT3 center;
    DirectX::XMFLOAT3 half;
    DirectX::XMFLOAT3 axis[3];

    DirectX::XMFLOAT3 rotation = {
        DirectX::XMConvertToRadians(angle.x),
        DirectX::XMConvertToRadians(angle.y),
        DirectX::XMConvertToRadians(angle.z)
    };

    // ÉÇÉfÉãÇÃ OBB ÇéÊìæ
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

void Gimmic_BreakWall::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    if (isBroken) return;
<<<<<<< HEAD

=======
    if (damage)
    {
        GameObject::Render(rc, renderer, { 1,0,0,1 });
    }
>>>>>>> parent of ad9ee60 („ÉÄ„É°„Éº„Ç∏„ÇíÂèó„Åë„Åü„ÇâÊüµ„ÅåËµ§Ëâ≤„Å´„Å™„Çã)
    GameObject::Render(rc, renderer);
}


void Gimmic_BreakWall::RenderDebugPrimitive(
    const RenderContext& rc, ShapeRenderer* renderer)
{
    if (isBroken) return;

    if (!renderer || !collider) return;

    DirectX::XMFLOAT3 pos;

    // OBB Å® AABB
    Collision::OBBtoAABB(
        box->center,
        box->half,
        box->axis,
        pos,
        size);

    DirectX::XMFLOAT3 angle = { 0,0,0 }; // AABB ÇÕâÒì]ÇµÇ»Ç¢
    DirectX::XMFLOAT4 color = { 0.2f, 0.8f, 0.2f, 1.0f };

    // AABB ògÇï`âÊ
    renderer->RenderBox(rc, pos, angle, size, color);
}

void Gimmic_BreakWall::OnImGui()
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
    }
}


REGISTER_GAMEOBJECT(Gimmic_BreakWall);

