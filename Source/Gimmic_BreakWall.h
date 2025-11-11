#pragma once
#include "GimmicBase.h"
#include "GimmicManager.h"
#include "Collider.h"
#include "System/ShapeRenderer.h"

class Gimmic_BreakWall : public GimmicBase
{
public:
	Gimmic_BreakWall();

	//�Փˌ���
	void OnCollision(GameObject* objects) override;

	//�M�~�b�N�X�V����
	void Update(float elapsedTime)override;

	//�`�揈��
	void Render(const RenderContext& rc, ModelRenderer* renderer) override;

	//�f�o�b�O�`��
	void RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer) override;

	void OnImGui();

	bool IsBroken() const { return isBroken; } // 攻撃対象判定用

private:
	bool isBroken = false;//��ꂽ���ǂ���
	float hp = 2.0f;//�ϋv�x

	float maxHp = 2.0f; // ���|�b�v���̂��߂ɍő�HP���L��
	float respawnTime = 5.0f; // ���|�b�v����܂ł̎��� (5�b)
	float respawnTimer = 0.0f; // ���|�b�v�^�C�}�[

	DirectX::XMFLOAT3 halfSize;
	DirectX::XMFLOAT3 size;

	OBB* box;
};

