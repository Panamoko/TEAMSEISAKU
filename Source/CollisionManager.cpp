#include "CollisionManager.h"

//オブジェクト登録
void CollisionManager::AddObject(GameObject* obj)
{
	if (obj)objects.push_back(obj);
}

//登録リストをクリア
void CollisionManager::Clear()
{
	objects.clear();
}

//全オブジェクト間の衝突判定
void CollisionManager::CheckAllCollision()
{
	//登録されている全オブジェクト同士の組み合わせチェック
	for (size_t i = 0; i < objects.size(); i++)
	{
		for (size_t j = i + 1; j < objects.size(), j++)
		{
			GameObject* objectA = objects[i];
			GameObject* objectB = objects[j];

			//無効なオブジェクトはスキップ
			if (!objectA->IsActive() || !objectB->IsActive())continue;
			if (!objectA->collider || !objectB->collider)continue;

			bool isCollisionDetected = false;

			//球 VS 球
			if (objectA->collider->type == ColliderType::Sphere &&
				objectB->collider->type == ColliderType::Sphere)
			{
				auto* sphereA = static_cast<SphereCollider*>(objectA->collider);
				auto* sphereB = static_cast<SphereCollider*>(objectB->collider);

				DirectX::XMFLOAT3 contactPoint;
				isCollisionDetected =
					Collision::IntersectSphereVsSphere(
						sphereA->center, sphereA->radius,
						sphereB->center, sphereB->radius,
						contactPoint);
			}

			//球 VS 箱
			else if (objectA->collider->type == ColliderType::Sphere &&
					 objectB->collider->type == ColliderType::Box)
			{
				auto* sphereA = static_cast<SphereCollider*>(objectA->collider);
				auto* boxB = static_cast<BoxCollider*>(objectB->collider);

				isCollisionDetected =
					Collision::IntersectSphereVsBox(
						sphereA->center, sphereA->radius,
						boxB->box_min, boxB->box_max);
			}

			//球 VS 円柱
			else if (objectA->collider->type == ColliderType::Sphere &&
					 objectB->collider->type == ColliderType::Cylinder)
			{
				auto* sphereA = static_cast<SphereCollider*>(objectA->collider);
				auto* cylinderB = static_cast<CylinderCollider*>(objectB->collider);

				DirectX::XMFLOAT3 contactPoint;
				isCollisionDetected =
					Collision::IntersectSphereVsCylinder(
						sphereA->center, sphereA->radius,
						cylinderB->center, cylinderB->radius, cylinderB->height,
						contactPoint);
			}

			//衝突した場合
			if (isCollisionDetected)
			{
			}
		}
	}
}
