#include "stdafx.h"
#include "Player.h"
#include "BossState.h"
#include "GameObject.h"
#include "RandomUtil.h"
#include "Terrain.h"
#include "Octree.h"

#include <iostream>
#include <cmath> // sqrt, pow �Լ� ���
#include <random>

constexpr float pi_f = 3.1415927f;

//=====================================Standing==============================================
void BossStandingState::Enter(std::shared_ptr<GameObject> npc)
{
	starttime = std::chrono::system_clock::now();
	duration_time = rand_time(dre) * 1500; // ������ �ð�(1~3��)�� �и��ʷ� ��ȯ
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

void BossStandingState::Execute(std::shared_ptr<GameObject> npc)
{
	endtime = std::chrono::system_clock::now();
	auto exectime = endtime - starttime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > duration_time) {
		// ���� ��ȯ
		npc->FSM_manager->ChangeState(std::make_shared<BossMoveState>());
		return;
	}

	//�ֺ��� �÷��̾ �ִ��� Ȯ��
	//�÷��̾ ������ Chase�� ����
	bool detected = false;
	{
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& pl : PlayerClient::PlayerClients) {
			if (pl.second->state != PC_INGAME) continue;
			auto playerInfo = pl.second;
			if (playerInfo) {
				XMFLOAT3 playerPos = playerInfo->GetPosition();
				XMFLOAT3 npcPos = npc->GetPosition();

				// �� ��ġ ������ 3D �Ÿ� ���
				float distance = sqrt(
					pow(playerPos.x - npcPos.x, 2) +
					pow(playerPos.y - npcPos.y, 2) +
					pow(playerPos.z - npcPos.z, 2));

				// 300 ���� ���� �ִٸ� Chase ���·� ��ȯ
				float detectionRange = 200.f;
				if (distance < detectionRange) {
					detected = true;
					break;
				}
			}
		}
	}
	if (detected) {
		npc->FSM_manager->ChangeState(std::make_shared<BossChaseState>());
		return;
	}
}

void BossStandingState::Exit(std::shared_ptr<GameObject> npc)
{
}
//=====================================Move=================================================

void BossMoveState::Enter(std::shared_ptr<GameObject> npc)
{
	starttime = std::chrono::system_clock::now();
	duration_time = rand_time(dre) * 1000; // ������ �ð�(1~3��)�� �и��ʷ� ��ȯ
	move_type = rand_type(dre); // ������ �̵� Ÿ��(0~2)
	rotate_type = rand_type(dre) % 2; // ������ ȸ�� Ÿ��(0~1)
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

void BossMoveState::Execute(std::shared_ptr<GameObject> npc)
{
	// ������ �̵�
	endtime = std::chrono::system_clock::now();
	auto exectime = endtime - starttime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > duration_time) {
		// ���� ��ȯ
		npc->FSM_manager->ChangeState(std::make_shared<BossStandingState>());
		return;
	}
	switch (move_type) {
	case 0:
		// ����
		npc->MoveForward(0.15f);
		break;
	case 1:
		// ȸ���ϸ鼭 ����
		npc->Rotate(0.f, 0.5f, 0.f);
		npc->MoveForward(0.1f);
		break;
	case 2:
		// ȸ��
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

	//�ֺ��� �÷��̾ �ִ��� Ȯ��
	//�÷��̾ ������ Chase�� ����
	bool detected = false;
	{
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& pl : PlayerClient::PlayerClients) {
			if (pl.second->state != PC_INGAME) continue;
			auto playerInfo = pl.second;
			if (playerInfo) {
				XMFLOAT3 playerPos = playerInfo->GetPosition();
				XMFLOAT3 npcPos = npc->GetPosition();

				// �� ��ġ ������ 3D �Ÿ� ���
				float distance = sqrt(
					pow(playerPos.x - npcPos.x, 2) +
					pow(playerPos.y - npcPos.y, 2) +
					pow(playerPos.z - npcPos.z, 2));

				// 300 ���� ���� �ִٸ� Chase ���·� ��ȯ
				float detectionRange = 200.f;
				if (distance < detectionRange) {
					detected = true;
					break;
				}
			}
		}
	}
	if (detected) {
		npc->FSM_manager->ChangeState(std::make_shared<BossChaseState>());
		return;
	}
}

void BossMoveState::Exit(std::shared_ptr<GameObject> npc)
{
}

//=====================================Chase=================================================

void BossChaseState::Enter(std::shared_ptr<GameObject> npc)
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
			aggro_player_id = t_obj->u_id;
		}
	}
}

void BossChaseState::Execute(std::shared_ptr<GameObject> npc)
{
	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, XMFLOAT3{500, 1000, 500}, results);
	if (results.size() <= 0) {
		npc->FSM_manager->ChangeState(std::make_shared<BossStandingState>());
		return;
	}


	int transition = 0; // 0 = none, 1 = attack, 2 = standing
	{
		std::lock_guard<std::mutex> lock(g_clients_mutex);
		for (auto& cl : PlayerClient::PlayerClients) {
			if (cl.second->m_id != aggro_player_id) continue;
			auto& player = cl.second;
			XMFLOAT3 playerPos = player->GetPosition();
			XMFLOAT3 npcPos = npc->GetPosition();
			// �÷��̾� ���� ���� ���
			XMVECTOR targetDirectionVec = XMVector3Normalize(XMVectorSet(playerPos.x - npcPos.x, 0.0f, playerPos.z - npcPos.z, 0.0f));
			XMFLOAT3 targetDirection;
			XMStoreFloat3(&targetDirection, targetDirectionVec);


			// NPC�� Look ���� ��������
			XMVECTOR npcLookVec = XMLoadFloat3(&npc->GetLook());
			XMFLOAT3 npcLookNorm;
			XMStoreFloat3(&npcLookNorm, XMVector3Normalize(npcLookVec)); // Look ���͵� ����ȭ
			XMVECTOR npcLookVecXZ = XMVector3Normalize(XMVectorSet(npcLookNorm.x, 0.0f, npcLookNorm.z, 0.0f));

			float targetYaw = atan2f(targetDirection.x, targetDirection.z);
			float currentYaw = atan2f(npcLookNorm.x, npcLookNorm.z);
			float deltaYaw = targetYaw - currentYaw;

			if (deltaYaw > pi_f)
				deltaYaw -= 2 * pi_f;
			else if (deltaYaw < -pi_f)
				deltaYaw += 2 * pi_f;

			// ��ǥ �������� ȸ��
			npc->Rotate(0.0f, deltaYaw * 2.f, 0.0f);

			float attackRange = 100.0f;
			if (BossAttackState::Sp_atk_counter > 5) {
				attackRange = 250.f;
			}
			float distanceToPlayer = sqrt(pow(playerPos.x - npcPos.x, 2) + pow(playerPos.y - npcPos.y, 2) + pow(playerPos.z - npcPos.z, 2));

			if (distanceToPlayer < attackRange) {
				const float BOSS_FOV_DEGREES = 90.0f;

				// �������� �÷��̾�� ���ϴ� ���� ���� (����ȭ)
				XMVECTOR toPlayerVec = XMVector3Normalize(XMVectorSet(playerPos.x - npcPos.x, playerPos.y - npcPos.y, playerPos.z - npcPos.z, 0.0f));
				// ������ ���� ���� ���� (����ȭ)
				XMVECTOR normalizedNpcLookVec = XMVector3Normalize(npcLookVec);

				// �� ������ ����(dot product) ���
				float dot = XMVectorGetX(XMVector3Dot(normalizedNpcLookVec, toPlayerVec));

				// �þ߰��� ����(half-angle)�� �ڻ��� �� ���
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
					// ���� ���·� ��ȯ�ϴ��� �Ʒ� ������ �������� �����Ƿ� ���⼭ return
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
			// �߰� �� ���� ���� (��: �÷��̾ �ʹ� �ָ� ���)
			float loseRange = 400.f;
			if (distanceToPlayer > loseRange) {
				transition = 2;
				break;
			}
			break;
		}
	}
	if (transition == 1) {
		if (npc->FSM_manager->GetAtkDelay() == false)
			npc->FSM_manager->ChangeState(std::make_shared<BossAttackState>());
		return;
	}
	if (transition == 2) {
		npc->FSM_manager->ChangeState(std::make_shared<BossStandingState>());
		return;
	}
}

void BossChaseState::Exit(std::shared_ptr<GameObject> npc)
{
}
//=====================================Die=================================================

void BossDieState::Enter(std::shared_ptr<GameObject> npc)
{
	starttime = std::chrono::system_clock::now();
	duration_time = 25 * 1000; // 25�ʰ� �׾�����

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

void BossDieState::Execute(std::shared_ptr<GameObject> npc)
{
	endtime = std::chrono::system_clock::now();
	auto exectime = endtime - starttime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > duration_time) {
		// ���� ��ȯ
		npc->FSM_manager->ChangeState(std::make_shared<BossRespawnState>());
		return;
	}
}

void BossDieState::Exit(std::shared_ptr<GameObject> npc)
{
}


//=====================================Respawn=================================================

void BossRespawnState::Enter(std::shared_ptr<GameObject> npc)
{
	npc->is_alive = false;
	starttime = std::chrono::system_clock::now();
	duration_time = 300 * 1000; // 5�а� �Ⱥ��̵���


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

void BossRespawnState::Execute(std::shared_ptr<GameObject> npc)
{
	endtime = std::chrono::system_clock::now();
	auto exectime = endtime - starttime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > duration_time) {
		// ���� ��ġ�� ����
		npc->Sethp(20);

		std::pair<float, float> randompos = genRandom::generateRandomXZ(gen, 1000.f, 2000.f, 1000.f, 2000.f);
		XMFLOAT3 xmf3Scale = Terrain::terrain->GetScale();
		int scale_z = (int)(randompos.second / xmf3Scale.z);
		bool bReverseQuad = ((scale_z % 2) != 0);
		float fHeight = Terrain::terrain->GetHeight(randompos.first, randompos.second, bReverseQuad) + 0.0f;
		float y{};
		if (y < fHeight) y = fHeight;
		if (npc->GetType() == OBJECT_TYPE::OB_BAT)
			npc->fly_height = 13.f;
		npc->SetPosition(randompos.first, y, randompos.second);

		Octree::GameObjectOctree.update(npc->GetID(), npc->GetPosition());

		// ���� ��ȯ
		npc->FSM_manager->ChangeState(std::make_shared<BossStandingState>());
		return;
	}
}

void BossRespawnState::Exit(std::shared_ptr<GameObject> npc)
{
	npc->is_alive = true;

	std::vector<tree_obj*> results;
	tree_obj n_obj{npc->GetID(), npc->GetPosition()};
	Octree::PlayerOctree.query(n_obj, oct_distance, results);
}

//=====================================Attack=================================================

int BossAttackState::Sp_atk_counter = 0;
void BossAttackState::Enter(std::shared_ptr<GameObject> npc)
{
	starttime = std::chrono::system_clock::now();
	duration_time = 2.65f * 1000; // 1�ʰ�
	Sp_atk_counter++;
	if (Sp_atk_counter > 5) {
		Sp_atk_counter = 0;
		npc->SetAnimationType(ANIMATION_TYPE::SPECIAL_ATTACK);
		duration_time = 2.f * 1000; // ����� ������ 2�ʰ�
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

void BossAttackState::Execute(std::shared_ptr<GameObject> npc)
{
	// ���ݸ�� �ð� üũ �� �ٽ� �����ϰ�
	endtime = std::chrono::system_clock::now();
	auto exectime = endtime - starttime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > duration_time) {
		npc->FSM_manager->ChangeState(std::make_shared<BossChaseState>());
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

void BossAttackState::Exit(std::shared_ptr<GameObject> npc)
{
	npc->FSM_manager->SetAtkDelay();
}

//=====================================Hit(�¾��� ���)=================================================

void BossHitState::Enter(std::shared_ptr<GameObject> npc)
{
	npc->SetAnimationType(ANIMATION_TYPE::HIT);
	starttime = std::chrono::system_clock::now();
	duration_time = 1.5f * 1000; // 1�ʰ� ����
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

void BossHitState::Execute(std::shared_ptr<GameObject> npc)
{
	endtime = std::chrono::system_clock::now();
	auto exectime = endtime - starttime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > duration_time) {
		npc->FSM_manager->ChangeState(std::make_shared<BossChaseState>());
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

void BossHitState::Exit(std::shared_ptr<GameObject> npc)
{
}


void BossGlobalState::Enter(std::shared_ptr<GameObject> npc)
{
}

void BossGlobalState::Execute(std::shared_ptr<GameObject> npc)
{
	if (is_invincible) {
		auto nowtime = std::chrono::system_clock::now();
		auto exectime = nowtime - starttime;
		auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
		if (exec_ms > sustainment_time) {
			is_invincible = false;
			std::vector<tree_obj*> results;
			tree_obj n_obj{npc->GetID(), npc->GetPosition()};
			Octree::PlayerOctree.query(n_obj, oct_distance, results);
			for (auto& p_obj : results) {
				std::lock_guard<std::mutex> lock(g_clients_mutex);
				for (auto& cl : PlayerClient::PlayerClients) {
					if (cl.second->state != PC_INGAME) continue;
					if (cl.second->m_id != p_obj->u_id) continue;
					cl.second->SendInvinciblePacket(npc->GetID(), is_invincible);
				}
			}
		}
	}
	if (is_atkdelay) {
		auto nowtime = std::chrono::system_clock::now();
		auto exectime = nowtime - atk_delay_starttime;
		auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
		if (exec_ms > 1.f * 1000) {
			is_atkdelay = false;
		}
	}
}

void BossGlobalState::Exit(std::shared_ptr<GameObject> npc)
{
}


void BossSpecialAttackStartState::Enter(std::shared_ptr<GameObject> npc)
{
	npc->SetAnimationType(ANIMATION_TYPE::GROUND_SPELL_START);
	starttime = std::chrono::system_clock::now();
	duration_time = 2.666f * 1000;
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

void BossSpecialAttackStartState::Execute(std::shared_ptr<GameObject> npc)
{
	endtime = std::chrono::system_clock::now();
	auto exectime = endtime - starttime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > duration_time) {
		npc->FSM_manager->ChangeState(std::make_shared<BossSpecialAttackEndState>());
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

void BossSpecialAttackStartState::Exit(std::shared_ptr<GameObject> npc)
{
}


void BossSpecialAttackEndState::Enter(std::shared_ptr<GameObject> npc)
{
	npc->SetAnimationType(ANIMATION_TYPE::GROUND_SPELL_END);
	starttime = std::chrono::system_clock::now();
	duration_time = 2.666f * 1000;
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

void BossSpecialAttackEndState::Execute(std::shared_ptr<GameObject> npc)
{
	endtime = std::chrono::system_clock::now();
	auto exectime = endtime - starttime;
	auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exectime).count();
	if (exec_ms > duration_time) {
		npc->FSM_manager->ChangeState(std::make_shared<BossStandingState>());
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

void BossSpecialAttackEndState::Exit(std::shared_ptr<GameObject> npc)
{
}
