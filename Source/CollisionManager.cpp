#include "CollisionManager.h"
#include "GameObject.h"

//オブジェクト登録
void CollisionManager::AddObject(GameObject* obj)
{
	if (!obj) return;

	// 重複登録回避
	if (std::find(objects.begin(), objects.end(), obj) == objects.end())
		objects.push_back(obj);
}

//登録リストをクリア
void CollisionManager::Clear()
{
	objects.clear();
}

//個別削除
void CollisionManager::Remove(GameObject* obj)
{
	if (!obj || objects.empty()) return;

	// すでに登録されていないなら何もしない
	auto it = std::find(objects.begin(), objects.end(), obj);
	bool exists = (it != objects.end());

	// 更新中は削除予約
	if (inUpdate)
	{
		// 二重追加を防止
		if (exists &&
			std::find(pendingRemovals.begin(), pendingRemovals.end(), obj) == pendingRemovals.end())
		{
			pendingRemovals.push_back(obj);
		}
		return;
	}

	// 更新中でない時の即時削除
	if (exists)
	{
		objects.erase(std::remove(objects.begin(), objects.end(), obj), objects.end());
	}
}


//全オブジェクト間の衝突判定
void CollisionManager::CheckAllCollision()
{
	inUpdate = true;

	//登録されている全オブジェクト同士の組み合わせチェック
	for (size_t i = 0; i < objects.size(); i++)
	{
		for (size_t j = i + 1; j < objects.size(); j++)
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
				SphereCollider* sphereA = static_cast<SphereCollider*>(objectA->collider.get());
				SphereCollider* sphereB = static_cast<SphereCollider*>(objectB->collider.get());

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
				SphereCollider* sphereA = static_cast<SphereCollider*>(objectA->collider.get());
				BoxCollider* boxB = static_cast<BoxCollider*>(objectB->collider.get());

				isCollisionDetected =
					Collision::IntersectSphereVsBox(
						sphereA->center, sphereA->radius,
						boxB->box_min, boxB->box_max);
			}

			//球 VS 円柱
			else if (objectA->collider->type == ColliderType::Sphere &&
					 objectB->collider->type == ColliderType::Cylinder)
			{
				SphereCollider* sphereA = static_cast<SphereCollider*>(objectA->collider.get());
				CylinderCollider* cylinderB = static_cast<CylinderCollider*>(objectB->collider.get());

				DirectX::XMFLOAT3 contactPoint;
				isCollisionDetected =
					Collision::IntersectSphereVsCylinder(
						sphereA->center, sphereA->radius,
						cylinderB->center, cylinderB->radius, cylinderB->height,
						contactPoint);
			}

			//球 VS OBB
			else if (objectA->collider->type == ColliderType::Sphere &&
					 objectB->collider->type == ColliderType::OBB)
			{
				SphereCollider* sphereA = static_cast<SphereCollider*>(objectA->collider.get());
				OBB* obb_B = static_cast<OBB*>(objectB->collider.get());
				isCollisionDetected = Collision::IntersectSphereVsOBB(
					sphereA->center, sphereA->radius, *obb_B);
			}

			//円柱　VS OBB
			else if (objectA->collider->type == ColliderType::Cylinder &&
					 objectB->collider->type == ColliderType::OBB)
			{
				CylinderCollider* cylinderA = static_cast<CylinderCollider*>(objectA->collider.get());
				OBB* obb_B = static_cast<OBB*>(objectB->collider.get());
				isCollisionDetected = Collision::IntersectCylinderVsOBB(
					cylinderA->center, cylinderA->radius, cylinderA->height, *obb_B, &mtd);
			}

			//円柱 VS AABB
			else if (objectA->collider->type == ColliderType::Cylinder &&
					 objectB->collider->type == ColliderType::Box)
			{
				CylinderCollider* cylinderA = static_cast<CylinderCollider*>(objectA->collider.get());
				BoxCollider* boxB = static_cast<BoxCollider*>(objectB->collider.get());
				isCollisionDetected = Collision::IntersectCylinderVsAABB(
					cylinderA, boxB);

			}

			////円柱　VS 円柱
			//else if (objectA->collider->type == ColliderType::Cylinder &&
			//		 objectB->collider->type == ColliderType::Cylinder)
			//{
			//	CylinderCollider* cylinderA = static_cast<CylinderCollider*>(objectA->collider.get());
			//	CylinderCollider* cylinderB = static_cast<CylinderCollider*>(objectB->collider.get());
			//	isCollisionDetected = Collision::IntersectCylinderVsCylinder(
			//		cylinderA->center, cylinderA->radius, cylinderA->height,
			//		cylinderB->center, cylinderB->radius, cylinderB->height,
			//		mtd
			//	);
			//}

			//衝突した場合
			if (isCollisionDetected)
			{
				objectA->OnCollision(objectB);
				objectB->OnCollision(objectA);
			}
		}
	}
	inUpdate = false;

	//まとめて削除
	ProcessPendingremovals();
}

void CollisionManager::ProcessPendingremovals()
{
	if (pendingRemovals.empty()) return;

	for (auto obj : pendingRemovals)
	{
		auto it = std::find(objects.begin(), objects.end(), obj);
		if (it != objects.end())
		{
			objects.erase(std::remove(objects.begin(), objects.end(), obj), objects.end());
		}
	}

	pendingRemovals.clear();
}
