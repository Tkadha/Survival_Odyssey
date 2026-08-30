#include "stdafx.h"
#include "Player.h"
#include "AtkState.h"
#include "GameObject.h"
#include "RandomUtil.h"
#include "Terrain.h"
#include "Octree.h"
#include <iostream>
#include <cmath> // sqrt, pow 함수 사용
#include <random>

constexpr float pi_f = 3.1415927f;

//=====================================Standing==============================================
void AtkNPCStandingState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = rand_time(dre) * 1000; // 랜덤한 시간(1~3초)을 밀리초로 변환
	npc->SetAnimationType(ANIMATION_TYPE::IDLE);

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendAnimationPacket(npc);
		}
	}
}

void AtkNPCStandingState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(AtkNPCMoveState::Instance());
		return;
	}

	//주변에 플레이어가 있는지 확인
	//플레이어가 있으면 Chase로 변경
	bool detected = false;
	{
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& pl : PlayerClient::PlayerClients) {
			if (pl.second->state != PC_INGAME) continue;
			auto playerInfo = pl.second;
			if (playerInfo) {
				XMFLOAT3 playerPos = playerInfo->GetPosition();
				XMFLOAT3 npcPos = npc->GetPosition();

				// 두 위치 사이의 3D 거리 계산
				float distance = sqrt(
					pow(playerPos.x - npcPos.x, 2) +
					pow(playerPos.y - npcPos.y, 2) +
					pow(playerPos.z - npcPos.z, 2));

				// 300 범위 내에 있다면 Chase 상태로 전환
				float detectionRange = 200.f;
				if (distance < detectionRange) {
					detected = true;
					break;
				}
			}
		}
	}
	if (detected) {
		npc->FSM_manager->ChangeState(AtkNPCChaseState::Instance());
		return;
	}
}

void AtkNPCStandingState::Exit(const std::shared_ptr<GameObject>& npc)
{
}


//=====================================Move=================================================

void AtkNPCMoveState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = rand_time(dre) * 1000; // 랜덤한 시간(1~3초)을 밀리초로 변환
	npc->m_fsmCtx.moveType = rand_type(dre); // 랜덤한 이동 타입(0~2)
	npc->m_fsmCtx.rotateType = rand_type(dre) % 2; // 랜덤한 회전 타입(0~1)
	npc->SetAnimationType(ANIMATION_TYPE::WALK);

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendAnimationPacket(npc);
		}
	}
}

void AtkNPCMoveState::Execute(const std::shared_ptr<GameObject>& npc)
{
	// 앞으로 이동
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(AtkNPCStandingState::Instance());
		return;
	}
	switch (npc->m_fsmCtx.moveType) {
	case 0:
		// 전진
		npc->MoveForward(0.2f);
		break;
	case 1:
		// 회전하면서 전진
		npc->Rotate(0.f, 0.5f, 0.f);
		npc->MoveForward(0.1f);
		break;
	case 2:
		// 회전
		npc->Rotate(0.f, 0.25f, 0.f);
		break;
	}
	Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendMovePacket(npc);
		}
	}

	//주변에 플레이어가 있는지 확인
	//플레이어가 있으면 Chase로 변경
	bool detected = false;
	{
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& pl : PlayerClient::PlayerClients) {
			if (pl.second->state != PC_INGAME) continue;
			auto playerInfo = pl.second;
			if (playerInfo) {
				XMFLOAT3 playerPos = playerInfo->GetPosition();
				XMFLOAT3 npcPos = npc->GetPosition();

				// 두 위치 사이의 3D 거리 계산
				float distance = sqrt(
					pow(playerPos.x - npcPos.x, 2) +
					pow(playerPos.y - npcPos.y, 2) +
					pow(playerPos.z - npcPos.z, 2));

				// 300 범위 내에 있다면 Chase 상태로 전환
				float detectionRange = 200.f;
				if (distance < detectionRange) {
					detected = true;
					break;
				}
			}
		}
	}
	if (detected) {
		npc->FSM_manager->ChangeState(AtkNPCChaseState::Instance());
		return;
	}
}

void AtkNPCMoveState::Exit(const std::shared_ptr<GameObject>& npc)
{
}

//=====================================Chase=================================================

void AtkNPCChaseState::Enter(const std::shared_ptr<GameObject>& npc)
{
	if (npc->GetType() == OBJECT_TYPE::OB_BAT) npc->fly_height = 13.f;
	npc->SetAnimationType(ANIMATION_TYPE::WALK);
	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendAnimationPacket(npc);
		}
	}
}

void AtkNPCChaseState::Execute(const std::shared_ptr<GameObject>& npc)
{
	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, XMFLOAT3{500, 1000, 500}, results);
	if (results.size() <= 0) {
		npc->FSM_manager->ChangeState(AtkNPCStandingState::Instance());
		return;
	}
	long long near_player_id{0};
	float near_player_distance{1000.f};
	for (auto& t_obj : results) {
		XMFLOAT3 playerPos = t_obj->position;
		XMFLOAT3 npcPos = npc->GetPosition();
		float distance = sqrt(
			pow(playerPos.x - npcPos.x, 2) +
			pow(playerPos.y - npcPos.y, 2) +
			pow(playerPos.z - npcPos.z, 2));
		if (distance < near_player_distance) {
			near_player_distance = distance;
			near_player_id = t_obj->u_id;
		}
	}


	int transition = 0; // 0 = none, 1 = attack, 2 = standing
	{
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->m_id != near_player_id) continue;

			XMFLOAT3 playerPos = cl.second->GetPosition();
			XMFLOAT3 npcPos = npc->GetPosition();
			// 플레이어 방향 벡터 계산
			XMVECTOR targetDirectionVec = XMVector3Normalize(XMVectorSet(playerPos.x - npcPos.x, 0.0f, playerPos.z - npcPos.z, 0.0f));
			XMFLOAT3 targetDirection;
			XMStoreFloat3(&targetDirection, targetDirectionVec);


			// NPC의 Look 벡터 가져오기
			XMFLOAT3 npcLook = npc->GetLook();
			XMVECTOR npcLookVec = XMLoadFloat3(&npcLook);
			XMFLOAT3 npcLookNorm;
			XMStoreFloat3(&npcLookNorm, XMVector3Normalize(npcLookVec)); // Look 벡터도 정규화

			// 수평면에서의 NPC Look 벡터 (Y 성분 0으로 설정 후 정규화)
			XMVECTOR npcLookVecXZ = XMVector3Normalize(XMVectorSet(npcLookNorm.x, 0.0f, npcLookNorm.z, 0.0f));

			// 목표 Yaw 값 계산 (수평 방향 벡터 사용)
			float targetYaw = atan2f(targetDirection.x, targetDirection.z);

			// 현재 NPC의 Yaw 값 계산 (수평 Look 벡터 사용)
			float currentYaw = atan2f(npcLookNorm.x, npcLookNorm.z);

			float deltaYaw = targetYaw - currentYaw;

			if (deltaYaw > pi_f) {
				deltaYaw -= 2 * pi_f;
			} else if (deltaYaw < -pi_f) {
				deltaYaw += 2 * pi_f;
			}
			// 목표 방향으로 회전
			npc->Rotate(0.0f, deltaYaw * 2, 0.0f);


			float attackRange = 30.0f;
			float distanceToPlayer = sqrt(pow(playerPos.x - npcPos.x, 2) + pow(playerPos.y - npcPos.y, 2) + pow(playerPos.z - npcPos.z, 2));

			if (distanceToPlayer < attackRange) {
				std::vector<tree_obj*> results;
				tree_obj n_obj{npc->GetID(), npc->GetPosition()};
				Octree::PlayerOctree.query(n_obj, oct_distance, results);
				for (auto& p_obj : results) {
					for (auto& cl : PlayerClient::PlayerClients) {
						if (cl.second->state != PC_INGAME) continue;
						if (cl.second->m_id != p_obj->u_id) continue;
						cl.second->SendMovePacket(npc);
					}
				}
				if (npc->m_fsmCtx.atkDelay == false) {
					transition = 1;
					break;
				}
			}
			if (distanceToPlayer > attackRange)
				npc->MoveForward(0.2f);
			Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());
			std::vector<tree_obj*> results;
			tree_obj n_obj{npc->GetID(), npc->GetPosition()};
			Octree::PlayerOctree.query(n_obj, oct_distance, results);
			for (auto& p_obj : results) {
				for (auto& cl : PlayerClient::PlayerClients) {
					if (cl.second->state != PC_INGAME) continue;
					if (cl.second->m_id != p_obj->u_id) continue;
					cl.second->SendMovePacket(npc);
				}
			}
			// 추격 중 멈춤 조건 (예: 플레이어가 너무 멀리 벗어남)
			float loseRange = 600.f;
			if (distanceToPlayer > loseRange) {
				transition = 2;
				break;
			}
			break;
		}
	}
	if (transition == 1) {
		npc->FSM_manager->ChangeState(AtkNPCAttackState::Instance());
		return;
	}
	if (transition == 2) {
		npc->FSM_manager->ChangeState(AtkNPCStandingState::Instance());
		return;
	}
}

void AtkNPCChaseState::Exit(const std::shared_ptr<GameObject>& npc)
{
}

//=====================================Die=================================================

void AtkNPCDieState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 10 * 1000; // 10초간 죽어있음

	npc->SetAnimationType(ANIMATION_TYPE::DIE);
	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendAnimationPacket(npc);
		}
	}
}

void AtkNPCDieState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(AtkNPCRespawnState::Instance());
		return;
	}
	if (npc->GetType() == OBJECT_TYPE::OB_BAT && npc->fly_height > 0.f) {
		npc->fly_height -= 1.f;
		if (npc->fly_height < 0.f) npc->fly_height = 0.f;
		npc->MoveForward(0.f);
	}
}

void AtkNPCDieState::Exit(const std::shared_ptr<GameObject>& npc)
{
}


//=====================================Respawn=================================================

void AtkNPCRespawnState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->is_alive = false;
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 20 * 1000; // 20초간 안보이도록


	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);

	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendRemovePacket(npc);
		}
	}
}

void AtkNPCRespawnState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 랜덤 위치에 생성
		npc->Sethp(20);

		std::pair<float, float> randompos = genRandom::generateRandomXZ(gen, 1000.f, 2000.f, 1000.f, 2000.f);
		XMFLOAT3 xmf3Scale = Terrain::terrain->GetScale();
		int scale_z = static_cast<int>(randompos.second / xmf3Scale.z);
		bool bReverseQuad = ((scale_z % 2) != 0);
		float fHeight = Terrain::terrain->GetHeight(randompos.first, randompos.second, bReverseQuad) + 0.0f;
		float y{};
		if (y < fHeight) y = fHeight;
		if (npc->GetType() == OBJECT_TYPE::OB_BAT)
			npc->fly_height = 13.f;
		npc->SetPosition(randompos.first, y, randompos.second);

		Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());

		// 상태 전환
		npc->FSM_manager->ChangeState(AtkNPCStandingState::Instance());
		return;
	}
}

void AtkNPCRespawnState::Exit(const std::shared_ptr<GameObject>& npc)
{
	npc->is_alive = true;

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
}

//=====================================Attack=================================================


void AtkNPCAttackState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 1.f * 1000; // 1초간
	npc->SetAnimationType(ANIMATION_TYPE::ATTACK);
	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendAnimationPacket(npc);
		}
	}
}

void AtkNPCAttackState::Execute(const std::shared_ptr<GameObject>& npc)
{
	// 공격모션 시간 체크 후 다시 추적하게
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		npc->FSM_manager->ChangeState(AtkNPCChaseState::Instance());
		return;
	}
	if (exec_ms < 0.25 * 1000.f) {
		auto n_type = npc->GetType();
		if (npc->fly_height > 0) npc->fly_height -= 0.5f; // 비행 중에는 조금씩 내려옴
		if (n_type == OBJECT_TYPE::OB_RAPTOR || n_type == OBJECT_TYPE::OB_TOAD)
			npc->MoveForward(0.5f);
		else
			npc->MoveForward(0.75f);

	} else {
		if (npc->GetType() == OBJECT_TYPE::OB_BAT && npc->fly_height < 13.f) {
			npc->fly_height += 0.5f;
			npc->MoveForward(0.f);
		}
	}
	Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;

			cl.second->SendMovePacket(npc);
		}
	}
}

void AtkNPCAttackState::Exit(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.atkDelay = true;
	npc->m_fsmCtx.atkDelayStart = std::chrono::system_clock::now();
}

//=====================================Hit(맞았을 경우)=================================================

void AtkNPCHitState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->SetAnimationType(ANIMATION_TYPE::HIT);
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 1.0f * 1000; // 1초간 진행
	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendAnimationPacket(npc);
		}
	}
}

void AtkNPCHitState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		npc->FSM_manager->ChangeState(AtkNPCChaseState::Instance());
		return;
	}
	if (exec_ms < 0.25 * 1000.f) {
		npc->MoveForward(-0.75f);
	}
	Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;

			cl.second->SendMovePacket(npc);
		}
	}
}

void AtkNPCHitState::Exit(const std::shared_ptr<GameObject>& npc)
{
}


void AtkNPCGlobalState::Enter(const std::shared_ptr<GameObject>& npc)
{
}

void AtkNPCGlobalState::Execute(const std::shared_ptr<GameObject>& npc)
{
	if (npc->m_fsmCtx.invincible) {
		auto nowtime = std::chrono::system_clock::now();
		auto exectime = nowtime - npc->m_fsmCtx.invincibleStart;
		auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
		if (exec_ms > npc->m_fsmCtx.invincibleDurationMs) {
			npc->m_fsmCtx.invincible = false;
			std::vector<tree_obj*> results;
			tree_obj n_obj{npc->GetID(), npc->GetPosition()};
			Octree::PlayerOctree.query(n_obj, oct_distance, results);
			for (auto& p_obj : results) {
				std::lock_guard<std::mutex> lock(g_clients_mutex);
				for (auto& cl : PlayerClient::PlayerClients) {
					if (cl.second->state != PC_INGAME) continue;
					if (cl.second->m_id != p_obj->u_id) continue;
					cl.second->SendInvinciblePacket(npc->GetID(), npc->m_fsmCtx.invincible);
				}
			}
		}
	}
	if (npc->m_fsmCtx.atkDelay) {
		auto nowtime = std::chrono::system_clock::now();
		auto exectime = nowtime - npc->m_fsmCtx.atkDelayStart;
		auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
		if (exec_ms > 1.f * 1000) {
			npc->m_fsmCtx.atkDelay = false;
		}
	}
}

void AtkNPCGlobalState::Exit(const std::shared_ptr<GameObject>& npc)
{
}
