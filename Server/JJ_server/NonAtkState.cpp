#include "stdafx.h"
#include "Player.h"
#include "NonAtkState.h"
#include "GameObject.h"
#include "RandomUtil.h"
#include "Terrain.h"
#include "Octree.h"
#include <iostream>


void NonAtkNPCGlobalState::Enter(const std::shared_ptr<GameObject>& npc)
{
}

void NonAtkNPCGlobalState::Execute(const std::shared_ptr<GameObject>& npc)
{
	if (npc->m_fsmCtx.invincible) {
		auto nowtime = std::chrono::system_clock::now();
		auto exectime = nowtime - npc->m_fsmCtx.invincibleStart;
		auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
		if (exec_ms > npc->m_fsmCtx.invincibleDurationMs) {
			npc->m_fsmCtx.invincible = false;

			std::vector<tree_obj*> results;
			XMFLOAT3 oct_distance{2500, 1000, 2500};
			tree_obj n_obj{npc->GetID(), npc->GetPosition()};
			Octree::PlayerOctree.query(n_obj, oct_distance, results);
			for (auto& p_obj : results) {
				std::lock_guard<mutex> lock(g_clients_mutex);
				for (auto& cl : PlayerClient::PlayerClients) {
					if (cl.second->state != PC_INGAME) continue;
					if (cl.second->m_id != p_obj->u_id) continue;
					cl.second->SendInvinciblePacket(npc->GetID(), npc->m_fsmCtx.invincible);
				}
			}
		}
	}
}

void NonAtkNPCGlobalState::Exit(const std::shared_ptr<GameObject>& npc)
{
}


//=====================================Standing=================================================


void NonAtkNPCStandingState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = rand_time(dre) * 1000; // 랜덤한 시간(1~3초)을 밀리초로 변환
	npc->SetAnimationType(ANIMATION_TYPE::IDLE);

	// 근처에 있는 플레이어에게 타입 보내기
	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendAnimationPacket(npc);
		}
	}
}

void NonAtkNPCStandingState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(NonAtkNPCMoveState::Instance());
		return;
	}
}

void NonAtkNPCStandingState::Exit(const std::shared_ptr<GameObject>& npc)
{
}


//=====================================Move=================================================


void NonAtkNPCMoveState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = rand_time(dre) * 1000; // 랜덤한 시간(1~3초)을 밀리초로 변환
	npc->m_fsmCtx.moveType = rand_type(dre); // 랜덤한 이동 타입(0~2)
	npc->m_fsmCtx.rotateType = rand_type(dre) % 2; // 랜덤한 회전 타입(0~1)
	npc->SetAnimationType(ANIMATION_TYPE::WALK);

	// 근처에 있는 플레이어에게 타입 보내기
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

void NonAtkNPCMoveState::Execute(const std::shared_ptr<GameObject>& npc)
{
	// 앞으로 이동
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(NonAtkNPCStandingState::Instance());
		return;
	}
	switch (npc->GetType()) {
	case OBJECT_TYPE::OB_COW: {
		switch (npc->m_fsmCtx.moveType) {
		case 0:
			// 전진
			npc->MoveForward(0.2f);
			break;
		case 1:
			// 회전하면서 전진
			if (npc->m_fsmCtx.rotateType == 0)
				npc->Rotate(0.f, -0.5f, 0.f);
			else if (npc->m_fsmCtx.rotateType == 1)
				npc->Rotate(0.f, 0.5f, 0.f);
			npc->MoveForward(0.1f);
			break;
		case 2:
			// 회전
			if (npc->m_fsmCtx.rotateType == 0)
				npc->Rotate(0.f, -0.25f, 0.f);
			else if (npc->m_fsmCtx.rotateType == 1)
				npc->Rotate(0.f, 0.25f, 0.f);
			break;
		}
	} break;
	case OBJECT_TYPE::OB_PIG: {
		switch (npc->m_fsmCtx.moveType) {
		case 0:
			// 전진
			npc->MoveForward(0.2f);
			break;
		case 1:
			// 회전하면서 전진
			if (npc->m_fsmCtx.rotateType == 0)
				npc->Rotate(0.f, -0.5f, 0.f);
			else if (npc->m_fsmCtx.rotateType == 1)
				npc->Rotate(0.f, 0.5f, 0.f);
			npc->MoveForward(0.1f);
			break;
		case 2:
			// 회전
			if (npc->m_fsmCtx.rotateType == 0)
				npc->Rotate(0.f, -0.25f, 0.f);
			else if (npc->m_fsmCtx.rotateType == 1)
				npc->Rotate(0.f, 0.25f, 0.f);
			break;
		}
	} break;
	default:
		break;
	}
	Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendMovePacket(npc);
		}
	}
}

void NonAtkNPCMoveState::Exit(const std::shared_ptr<GameObject>& npc)
{
}

//=====================================RunAway=================================================

void NonAtkNPCRunAwayState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.runAwayTotalMs = 0; // 총 시간 초기화
	npc->m_fsmCtx.stateDurationMs = 10 * 1000; // 10초간 도망다님
	npc->m_fsmCtx.moveType = rand_type(dre) % 2; // 랜덤한 이동 타입(0~1)
	npc->m_fsmCtx.rotateType = rand_type(dre) % 2; // 랜덤한 회전 타입(0~1)
	npc->SetAnimationType(ANIMATION_TYPE::RUN);

	// 근처에 있는 플레이어에게 타입 보내기
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

void NonAtkNPCRunAwayState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (npc->m_fsmCtx.runAwayTotalMs > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(NonAtkNPCMoveState::Instance());
		return;
	}
	switch (npc->GetType()) {
	case OBJECT_TYPE::OB_COW: {
		switch (npc->m_fsmCtx.moveType) {
		case 0:
			// 전진
			npc->MoveForward(0.45f);
			break;
		case 1:
			// 회전하면서 전진
			if (npc->m_fsmCtx.rotateType == 0)
				npc->Rotate(0.f, -1.0f, 0.f);
			else if (npc->m_fsmCtx.rotateType == 1)
				npc->Rotate(0.f, 1.0f, 0.f);
			npc->MoveForward(0.3f);
			break;
		}
	} break;
	case OBJECT_TYPE::OB_PIG: {
		switch (npc->m_fsmCtx.moveType) {
		case 0:
			// 전진
			npc->MoveForward(0.6f);
			break;
		case 1:
			// 회전하면서 전진
			if (npc->m_fsmCtx.rotateType == 0)
				npc->Rotate(0.f, -1.0f, 0.f);
			else if (npc->m_fsmCtx.rotateType == 1)
				npc->Rotate(0.f, 1.0f, 0.f);
			npc->MoveForward(0.45f);
			break;
		}
	} break;
	default:
		break;
	}
	Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());
	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
	for (auto& p_obj : results) {
		std::lock_guard<mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendMovePacket(npc);
		}
	}

	if (exec_ms > npc->m_fsmCtx.stateDurationMs / 20) {
		npc->m_fsmCtx.moveType = rand_type(dre) % 2;
		npc->m_fsmCtx.rotateType = rand_type(dre) % 2;
		npc->m_fsmCtx.runAwayTotalMs += exec_ms;
		npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	}
}

void NonAtkNPCRunAwayState::Exit(const std::shared_ptr<GameObject>& npc)
{
}

//=====================================Die=================================================

void NonAtkNPCDieState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 10 * 1000; // 10초간 죽어있음

	npc->SetAnimationType(ANIMATION_TYPE::DIE);
	// 근처에 있는 플레이어에게 타입 보내기
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

void NonAtkNPCDieState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		// 상태 전환
		npc->FSM_manager->ChangeState(NonAtkNPCRespawnState::Instance());
		return;
	}
}

void NonAtkNPCDieState::Exit(const std::shared_ptr<GameObject>& npc)
{
}
//=====================================Respawn=================================================

void NonAtkNPCRespawnState::Enter(const std::shared_ptr<GameObject>& npc)
{
	npc->is_alive = false;
	npc->m_fsmCtx.stateEnterTime = std::chrono::system_clock::now();
	npc->m_fsmCtx.stateDurationMs = 20 * 1000; // 20초간 안보이도록

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);

	for (auto& p_obj : results) {
		std::lock_guard<mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->state != PC_INGAME) continue;
			if (cl.second->m_id != p_obj->u_id) continue;
			cl.second->SendRemovePacket(npc);
		}
	}
}

void NonAtkNPCRespawnState::Execute(const std::shared_ptr<GameObject>& npc)
{
	auto endtime = std::chrono::system_clock::now();
	auto exectime = endtime - npc->m_fsmCtx.stateEnterTime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > npc->m_fsmCtx.stateDurationMs) {
		npc->Sethp(20);
		std::pair<float, float> randompos = genRandom::generateRandomXZ(gen, 1000.f, 2000.f, 1000.f, 2000.f);
		float fHeight = Terrain::terrain->GetHeightAtQuad(randompos.first, randompos.second);
		float y{};
		if (y < fHeight) y = fHeight;

		npc->SetPosition(randompos.first, y, randompos.second);

		Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());

		// 상태 전환
		npc->FSM_manager->ChangeState(NonAtkNPCStandingState::Instance());
		return;
	}
}

void NonAtkNPCRespawnState::Exit(const std::shared_ptr<GameObject>& npc)
{
	npc->is_alive = true;

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
}
