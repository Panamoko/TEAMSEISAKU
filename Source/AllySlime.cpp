//
// AllySlime.cpp
//

#include "AllySlime.h"
#include "Character.h"   // �� �����œ����i�w�b�_����͊O�����j
#include "Player.h"              // �v���C���[�ʒu�E�p�x���擾���ĕґ��A���J�[���o��
#include "EnemyManager.h"        // �G�̎擾
#include "ProjectileStraite.h"   // ���i�e�i�v���C���[�Ɠ������̂��g�p�j
#include "ModelManager.h"        // ���f�����[�h
#include "Collision.h"           // ���~�~���Ȃǂ̓����蔻��
#include "AllyTargeting.h"
#include "GimmicManager.h"        // BreakWallとCoreを取得するため
#include "Gimmic_BreakWall.h"    // BreakWall判定用
#include "Core.h"                // Core判定用   // �� �ǉ�
#include <cmath>
#include <cfloat>

using namespace DirectX;

// ���傢�֗��ȉ��Z�q�iXMFLOAT3 �͉��Z�q���������Ƃ������̂Ŏ��O�ŗp�Ӂj
static inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline XMFLOAT3 operator*(const XMFLOAT3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
// --- �R�A�擾�i���Ȃ��̊��ɍ��킹�Ăǂ��炩�j ---
AllySlime::AllySlime(int formationIndex)
    : index(formationIndex)
{
    // �����ڂ͓G�X���C���𗬗p�iEnemySlime �Ɠ����p�X�^�X�P�[���ɑ�����j
    slimeModel = ModelManager::Instance().Load("Data/Model/Slime/suraimukari.mdl");
    scale = { 0.002f, 0.002f, 0.002f }; // ���f�����傫���O��̂��ߏk��
    radius = 0.5f;                  // �����蔼�a�i�G�Ɠ����j
    height = 1.0f;                  // �����荂��

    // �����ʒu�̓v���C���[�̋߂��i���ۂ̐���� UpdateAnchor �ōs���j
    {
        const Player& ref = (leader ? *leader : Player::Instance());
        position = ref.GetPosition();
    }
    UpdateTransform();
}

void AllySlime::UpdateAnchor()
{
    // �v���C���[�̈ʒu�E�p�x�iY=���[�p�j���擾
    const Player& ref = (leader ? *leader : Player::Instance());
    const XMFLOAT3& p = ref.GetPosition();
    const XMFLOAT3& a = ref.GetAngle();

    // �E����W�n�F�O��=+Z �Ƃ��āA���[�p����O��/�E�x�N�g�����Z�o
    XMFLOAT3 fwd = { std::sinf(a.y), 0.0f,  std::cosf(a.y) };
    XMFLOAT3 rgt = { std::cosf(a.y), 0.0f, -std::sinf(a.y) };

    // ���g�� index ���� �g�s�i�c�j/��i���j�h ������
    const int row = index / rowWidth;  // ���i�ڂ��i���ɍs���قǒl���傫���j
    const int col = index % rowWidth;  // ���̒i�̉���ڂ��i���E�j

    // �������̒�����ɕ��ׂ邽�߁A-(rowWidth-1)/2..+(rowWidth-1)/2 �ɕ��s�ړ�
    const float rightOffset = (col - (rowWidth - 1) * 0.5f) * lateralSpacing;
    const float backOffset = (row + 1) * followDistance; // �v���C���[�g����h�Ȃ̂� +back �� -fwd ���Ɏ��

    // �ڕW�A���J�[���W�F�v���C���[�ʒu + �E�~rightOffset - �O�~backOffset
    anchor = p + (rgt * rightOffset) + (fwd * (-backOffset));
    anchor.y = p.y; // �n�ʊ�ɌŒ�i�i���Ή����K�v�Ȃ�ʓr���C�L���X�g�Ȃǂ������j
}

void AllySlime::AutoAttackUpdate(float elapsedTime)
{
    if (!autoAttackEnabled) return;

    // �A�˃N�[���_�E��
    if (autoAttackTimer > 0.0f) {
        autoAttackTimer -= elapsedTime;
        return;
    }

    // �˒����́g�Ŋ��h�̓G��T��
    EnemyManager& em = EnemyManager::Instance();
    const XMFLOAT3 pos = { position.x, position.y + height * 0.5f, position.z };
    const float rangeSq = autoAttackRange * autoAttackRange;

    float bestDistSq = FLT_MAX;
    XMFLOAT3 bestTarget = { 0, 0, 0 };
    bool hasTarget = false;

    const int enemyCount = em.GetEnemyCount();
    for (int i = 0; i < enemyCount; ++i) {
        std::shared_ptr<Enemy> enemy = em.GetEnemy(i);
        if (!enemy) continue;

        // �������S���G�㔼�g������܂ł̋����iY�͏㔼�g���m�ō��킹��j
        const XMFLOAT3& ep = enemy->GetPosition();
        float dx = ep.x - position.x;
        float dy = (ep.y + enemy->GetHeight() * 0.5f) - (position.y + height * 0.5f);
        float dz = ep.z - position.z;
        const float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= rangeSq && distSq < bestDistSq) {
            bestDistSq = distSq;
            bestTarget = ep;
            bestTarget.y += enemy->GetHeight() * 0.5f;
            hasTarget = true;
        }
    }
    GimmicManager& gm = GimmicManager::Instance();
    auto& gimmicks = gm.GetAll();
    for (auto& gimmic : gimmicks) {
        if (!gimmic) continue;
        if (!gimmic->IsActive()) continue;

        bool isTarget = false;
        float targetHeight = 0.0f;

        std::shared_ptr<GimmicBase> gimmicPtr = gimmic; // 参照を保持
        if (!gimmicPtr) continue;

        if (gimmicPtr->class_name == "Gimmic_BreakWall") {
            Gimmic_BreakWall* breakWall = dynamic_cast<Gimmic_BreakWall*>(gimmicPtr.get());
            if (breakWall && !breakWall->IsBroken()) {
                isTarget = true;
                targetHeight = gimmicPtr->scale.y * 2.0f;
            }
        }
        else if (gimmicPtr->class_name == "Core") {
            Core* core = dynamic_cast<Core*>(gimmicPtr.get());
            if (core && core->GetHP() > 0.0f) {
                isTarget = true;
                targetHeight = 7.0f;
            }
        }

        if (isTarget) {
            const XMFLOAT3& gp = gimmicPtr->position;
            float dx = gp.x - position.x;
            float dy = (gp.y + targetHeight * 0.5f) - (position.y + height * 0.5f);
            float dz = gp.z - position.z;
            const float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq <= rangeSq && distSq < bestDistSq) {
                bestDistSq = distSq;
                bestTarget = gp;
                bestTarget.y += targetHeight * 0.5f;
                hasTarget = true;
            }
        }
    }

    if (!hasTarget) return; // �˒����ɂ��Ȃ�

    // ���ˈʒu�i�������炢�j
    

    // �G�i�㔼�g�j�����̐��K���x�N�g�����Z�o
    XMFLOAT3 dir = { bestTarget.x - pos.x, bestTarget.y - pos.y, bestTarget.z - pos.z };
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    // ���i�e�𐶐������ˁiPlayer �Ɠ��� ProjectileStraite ���g�p�j
    auto* proj = new ProjectileStraite(&projectileManager); // �Ǘ��͎��O�� projectileManager
    proj->Launch(dir, pos);

    // �N�[���_�E���J�n
    autoAttackTimer = autoAttackInterval;
}

void AllySlime::CollisionProjectilesVsEnemies()
{
    // �����̒e vs �G�̉~�������蔻��
    EnemyManager& em = EnemyManager::Instance();

    const int projectileCount = projectileManager.GetProjectileCount();
    const int enemyCount = em.GetEnemyCount();

    for (int i = 0; i < projectileCount; ++i) {
        Projectile* projectile = projectileManager.GetProjectile(i);
        if (!projectile) continue; // ���ɔj���ς݃X���b�g�Ȃ�

        for (int j = 0; j < enemyCount; ++j) {
            std::shared_ptr<Enemy> enemy = em.GetEnemy(j);
            if (!enemy) continue;

            XMFLOAT3 outPos;
            // ���i�e�j�~�~���i�G�j����F�v���W�F�N�g�̋��� Collision ���g�p
            const bool hit = Collision::IntersectSphereVsCylinder(
                projectile->GetPosition(),
                projectile->GetRadius(),
                enemy->GetPosition(),
                enemy->GetRadius(),
                enemy->GetHeight(),
                outPos);

            if (!hit) continue;

            // �q�b�g�F�_���[�W�i���G0.5s�� Player �Ƒ����j
            if (enemy->ApplyDamage(1, 0.5f)) {
                // �y���m�b�N�o�b�N�iXZ ���ʊ�j
                XMFLOAT3 impulse{};
                const float power = 10.0f;
                const XMFLOAT3& e = enemy->GetPosition();
                const XMFLOAT3& p = projectile->GetPosition();
                float vx = e.x - p.x;
                float vz = e.z - p.z;
                const float lenXZ = std::sqrt(vx * vx + vz * vz);
                if (lenXZ > 0.0001f) { vx /= lenXZ; vz /= lenXZ; }
                impulse.x = vx * power;
                impulse.y = power * 0.5f; // ������������
                impulse.z = vz * power;
                enemy->AddImpulse(impulse);
            }

            // �e�͈�̂ɓ���������j��
            projectile->Destroy();
            break;
        }

        // 2) BreakWallとCoreとの衝突判定
        if (!projectile) continue;

        GimmicManager& gm = GimmicManager::Instance();
        auto& gimmicks = gm.GetAll();
        for (auto& gimmic : gimmicks) {
            if (!gimmic) continue;
            if (!gimmic->IsActive()) continue;

            std::shared_ptr<GimmicBase> gimmicPtr = gimmic; // 参照を保持
            if (!gimmicPtr) continue;

            if (gimmicPtr->class_name == "Gimmic_BreakWall") {
                Gimmic_BreakWall* breakWall = dynamic_cast<Gimmic_BreakWall*>(gimmicPtr.get());
                if (!breakWall || breakWall->IsBroken()) continue;

                if (gimmicPtr->collider && gimmicPtr->collider->type == ColliderType::OBB) {
                    OBB* obb = static_cast<OBB*>(gimmicPtr->collider.get());
                    XMFLOAT3 outMTD;
                    if (Collision::IntersectSphereVsOBB(
                        projectile->GetPosition(),
                        projectile->GetRadius(),
                        *obb,
                        &outMTD))
                    {
                        breakWall->OnCollision(projectile);
                        projectile->Destroy();
                        break;
                    }
                }
            }
            else if (gimmicPtr->class_name == "Core") {
                Core* core = dynamic_cast<Core*>(gimmicPtr.get());
                if (!core || core->GetHP() <= 0.0f) continue;

                if (gimmicPtr->collider && gimmicPtr->collider->type == ColliderType::Cylinder) {
                    CylinderCollider* cylinder = static_cast<CylinderCollider*>(gimmicPtr->collider.get());
                    XMFLOAT3 outPos;
                    if (Collision::IntersectSphereVsCylinder(
                        projectile->GetPosition(),
                        projectile->GetRadius(),
                        cylinder->center,
                        cylinder->radius,
                        cylinder->height,
                        outPos))
                    {
                        core->OnCollision(projectile);
                        projectile->Destroy();
                        break;
                    }
                }
            }
        }
    }
}

void AllySlime::Update(float elapsedTime)
{
    // 1) �ґ��A���J�[�i�v���C���[����̖ڕW�j���X�V
    UpdateAnchor();

    // 2) �ڕW�A���J�[�ֈړ����邽�߂̓��̓x�N�g�����Z�o�iXZ �̂݁j
    float vx = anchor.x - position.x;
    float vz = anchor.z - position.z;
    float d = std::sqrt(vx * vx + vz * vz);
    if (d > 0.0001f) { vx /= d; vz /= d; }
    else { vx = vz = 0.0f; }

    // 3) ������ Character API �𗘗p���āA���R�ȉ���/����������ŒǏ]
    Move(elapsedTime, vx, vz, moveSpeed);
    Turn(elapsedTime, vx, vz, turnSpeed);

    // 4) �����U���i���˃^�C�~���O�̊Ǘ��ƒe�����j
    AutoAttackUpdate(elapsedTime);

    // 5) ���x�X�V�E���G�^�C�}�[�E���[���h�s��̍X�V�iCharacter ���̕W�������j
    UpdateVelocity(elapsedTime);
    UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

    // 6) �����̒e�̍X�V���G�Ƃ̓����蔻��
    projectileManager.Update(elapsedTime);
    CollisionProjectilesVsEnemies();
}

void AllySlime::Render(const RenderContext& rc, ModelRenderer* renderer)
{
    // �{�̃��f���̕`��iLambert ���A�G�X���C���Ƒ�����j
    renderer->Render(rc, transform, slimeModel, ShaderId::Lambert);

    // �����̒e�̕`��
    projectileManager.Render(rc, renderer);
}

void AllySlime::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    // Character ���̃f�o�b�O������i�~���Ȃǁj
    Character::RenderDebugPrimitive(rc, renderer);

    // �����̒e�̃f�o�b�O�`��
    projectileManager.RenderDebugPrimitive(rc, renderer);

    // �˒��̉����iON���̂݁j�F�΂̔������~��
    if (autoAttackEnabled) {
        const XMFLOAT3 center = { position.x, position.y, position.z };
        renderer->RenderCylinder(rc, center, autoAttackRange, height, XMFLOAT4(0, 1, 0, 0.2f));
    }

    // �Ǐ]�A���J�[�̖ڈ�i���F�̏����j
    renderer->RenderSphere(rc, anchor, 0.15f, XMFLOAT4(1, 1, 0, 1));
}
