#include "stdafx.h"
#include "Player.h"
#include "BossState.h"
#include "GameObject.h"
#include "RandomUtil.h"
#include "Terrain.h"
#include "Octree.h"

#include <iostream>
#include <cmath> // sqrt, pow 함수 사용
#include <random>

constexpr float pi_f = 3.1415927f;

//=====================================Standing==============================================
void BossStandingState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = rand_time(dre) * 1500; // 랜덤한 시간(1~3초)을 밀리초로 변환
	npc->SetAnimationType(ANIMATION_TYPE::IDLE);

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendAnimationPacket(npc);
		}
	}
}

void BossStandingState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(BossMoveState::Instance());
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
		npc->FSM_manager->ChangeState(BossChaseState::Instance());
		return;
	}
}

void BossStandingState::Exit(const std::shared_ptr<GameObject>& npc)
{
}
//=====================================Move=================================================

void BossMoveState::Enter(const std::shared_ptr<GameObject>& npc)
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
		std::lock_guard<mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendAnimationPacket(npc);
		}
	}
}

void BossMoveState::Execute(const std::shared_ptr<GameObject>& npc)
{
	// 앞으로 이동
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(BossStandingState::Instance());
		return;
	}
	switch (npc->m_fsmCtx.moveType) {
	case 0:
		// 전진
		npc->MoveForward(0.15f);
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
		npc->FSM_manager->ChangeState(BossChaseState::Instance());
		return;
	}
}

void BossMoveState::Exit(const std::shared_ptr<GameObject>& npc)
{
}

//=====================================Chase=================================================

void BossChaseState::Enter(const std::shared_ptr<GameObject>& npc)
{
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
			npc->m_fsmCtx.aggroPlayerId = t_obj->u_id;
		}
	}
}

void BossChaseState::Execute(const std::shared_ptr<GameObject>& npc)
{
	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, XMFLOAT3{500, 1000, 500}, results);
	if (results.size() <= 0) {
		npc->FSM_manager->ChangeState(BossStandingState::Instance());
		return;
	}


	int transition = 0; // 0 = none, 1 = attack, 2 = standing
	{
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->m_id != npc->m_fsmCtx.aggroPlayerId) continue;
			auto& player = cl.second;
			XMFLOAT3 playerPos = player->GetPosition();
			XMFLOAT3 npcPos = npc->GetPosition();
			// 플레이어 방향 벡터 계산
			XMVECTOR targetDirectionVec = XMVector3Normalize(XMVectorSet(playerPos.x - npcPos.x, 0.0f, playerPos.z - npcPos.z, 0.0f));
			XMFLOAT3 targetDirection;
			XMStoreFloat3(&targetDirection, targetDirectionVec);


			// NPC의 Look 벡터 가져오기
			XMVECTOR npcLookVec = XMLoadFloat3(&npc->GetLook());
			XMFLOAT3 npcLookNorm;
			XMStoreFloat3(&npcLookNorm, XMVector3Normalize(npcLookVec)); // Look 벡터도 정규화
			XMVECTOR npcLookVecXZ = XMVector3Normalize(XMVectorSet(npcLookNorm.x, 0.0f, npcLookNorm.z, 0.0f));

			float targetYaw = atan2f(targetDirection.x, targetDirection.z);
			float currentYaw = atan2f(npcLookNorm.x, npcLookNorm.z);
			float deltaYaw = targetYaw - currentYaw;

			if (deltaYaw > pi_f)
				deltaYaw -= 2 * pi_f;
			else if (deltaYaw < -pi_f)
				deltaYaw += 2 * pi_f;

			// 목표 방향으로 회전
			npc->Rotate(0.0f, deltaYaw * 2.f, 0.0f);

			float attackRange = 100.0f;
			if (npc->m_fsmCtx.spAtkCounter > 5) {
				attackRange = 250.f;
			}
			float distanceToPlayer = sqrt(pow(playerPos.x - npcPos.x, 2) + pow(playerPos.y - npcPos.y, 2) + pow(playerPos.z - npcPos.z, 2));

			if (distanceToPlayer < attackRange) {
				const float BOSS_FOV_DEGREES = 90.0f;

				// 보스에서 플레이어로 향하는 방향 벡터 (정규화)
				XMVECTOR toPlayerVec = XMVector3Normalize(XMVectorSet(playerPos.x - npcPos.x, playerPos.y - npcPos.y, playerPos.z - npcPos.z, 0.0f));
				// 보스의 정면 방향 벡터 (정규화)
				XMVECTOR normalizedNpcLookVec = XMVector3Normalize(npcLookVec);

				// 두 벡터의 내적(dot product) 계산
				float dot = XMVectorGetX(XMVector3Dot(normalizedNpcLookVec, toPlayerVec));

				// 시야각의 절반(half-angle)의 코사인 값 계산
				float cosHalfFov = cosf(XMConvertToRadians(BOSS_FOV_DEGREES / 2.0f));

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

				if (dot >= cosHalfFov) {
					transition = 1;
					break;
				}
			}

			npc->MoveForward(0.1f);

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
			float loseRange = 400.f;
			if (distanceToPlayer > loseRange) {
				transition = 2;
				break;
			}
			break;
		}
	}
	if (transition == 1) {
		if (npc->m_fsmCtx.atkDelay == false)
			npc->FSM_manager->ChangeState(BossAttackState::Instance());
		return;
	}
	if (transition == 2) {
		npc->FSM_manager->ChangeState(BossStandingState::Instance());
		return;
	}
}

void BossChaseState::Exit(const std::shared_ptr<GameObject>& npc)
{
}
//=====================================Die=================================================

void BossDieState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 25 * 1000; // 25초간 죽어있음

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

void BossDieState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(BossRespawnState::Instance());
		return;
	}
}

void BossDieState::Exit(const std::shared_ptr<GameObject>& npc)
{
}


//=====================================Respawn=================================================

void BossRespawnState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->is_alive = false;
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 300 * 1000; // 5분간 안보이도록


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

void BossRespawnState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 랜덤 위치에 생성
		npc->Sethp(20);

		std::pair<float, float> randompos = genRandom::generateRandomXZ(gen, 1000.f, 2000.f, 1000.f, 2000.f);
		float fHeight = Terrain::terrain->GetHeightAtQuad(randompos.first, randompos.second);
		float y{};
		if (y < fHeight) y = fHeight;
		if (npc->GetType() == OBJECT_TYPE::OB_BAT)
			npc->fly_height = 13.f;
		npc->SetPosition(randompos.first, y, randompos.second);

		Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());

		// 상태 전환
		npc->FSM_manager->ChangeState(BossStandingState::Instance());
		return;
	}
}

void BossRespawnState::Exit(const std::shared_ptr<GameObject>& npc)
{
	npc->is_alive = true;

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
}

//=====================================Attack=================================================

void BossAttackState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 2.65f * 1000; // 1초간
	npc->m_fsmCtx.spAtkCounter++;
	if (npc->m_fsmCtx.spAtkCounter > 5) {
		npc->m_fsmCtx.spAtkCounter = 0;
		npc->SetAnimationType(ANIMATION_TYPE::SPECIAL_ATTACK);
		npc->m_fsmCtx.stateDurationMs = 2.f * 1000; // 스페셜 공격은 2초간
	} else {
		npc->SetAnimationType(ANIMATION_TYPE::ATTACK);
	}

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

void BossAttackState::Execute(const std::shared_ptr<GameObject>& npc)
{
	// 공격모션 시간 체크 후 다시 추적하게
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		npc->FSM_manager->ChangeState(BossChaseState::Instance());
		return;
	}
	if (exec_ms < 0.25 * 1000.f) {
		auto n_type = npc->GetAnimationType();
		if (n_type == ANIMATION_TYPE::SPECIAL_ATTACK)
			npc->MoveForward(0.1f);
		else
			npc->MoveForward(0.5f);
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

void BossAttackState::Exit(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.atkDelay = true;
	npc->m_fsmCtx.atkDelayStart = std::chrono::system_clock::now();
}

//=====================================Hit(맞았을 경우)=================================================

void BossHitState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->SetAnimationType(ANIMATION_TYPE::HIT);
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 1.5f * 1000; // 1초간 진행
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

void BossHitState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		npc->FSM_manager->ChangeState(BossChaseState::Instance());
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

void BossHitState::Exit(const std::shared_ptr<GameObject>& npc)
{
}


void BossGlobalState::Enter(const std::shared_ptr<GameObject>& npc)
{
}

void BossGlobalState::Execute(const std::shared_ptr<GameObject>& npc)
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

void BossGlobalState::Exit(const std::shared_ptr<GameObject>& npc)
{
}


void BossSpecialAttackStartState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->SetAnimationType(ANIMATION_TYPE::GROUND_SPELL_START);
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 2.666f * 1000;
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

void BossSpecialAttackStartState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		npc->FSM_manager->ChangeState(BossSpecialAttackEndState::Instance());
		return;
	}
	npc->MoveForward(0.f);

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

void BossSpecialAttackStartState::Exit(const std::shared_ptr<GameObject>& npc)
{
}


void BossSpecialAttackEndState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->SetAnimationType(ANIMATION_TYPE::GROUND_SPELL_END);
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 2.666f * 1000;
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

void BossSpecialAttackEndState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		npc->FSM_manager->ChangeState(BossStandingState::Instance());
		return;
	}

	npc->MoveForward(0.f);

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

void BossSpecialAttackEndState::Exit(const std::shared_ptr<GameObject>& npc)
{
}
