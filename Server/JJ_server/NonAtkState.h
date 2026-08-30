#pragma once
#include "FSMState.h"
class GameObject;

// 비공격형 NPC(소/돼지) 상태.
// 각 상태는 인스턴스 1개(Instance)만 공유하며 데이터 멤버가 없다.
// 타이머/이동타입 등 객체별 값은 GameObject::m_fsmCtx 사용.

class NonAtkNPCGlobalState : public FSMState<GameObject>
{
public:
	static NonAtkNPCGlobalState* Instance()
	{
		static NonAtkNPCGlobalState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class NonAtkNPCStandingState : public FSMState<GameObject>
{
public:
	static NonAtkNPCStandingState* Instance()
	{
		static NonAtkNPCStandingState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class NonAtkNPCMoveState : public FSMState<GameObject>
{
public:
	static NonAtkNPCMoveState* Instance()
	{
		static NonAtkNPCMoveState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class NonAtkNPCRunAwayState : public FSMState<GameObject>
{
public:
	static NonAtkNPCRunAwayState* Instance()
	{
		static NonAtkNPCRunAwayState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class NonAtkNPCDieState : public FSMState<GameObject>
{
public:
	static NonAtkNPCDieState* Instance()
	{
		static NonAtkNPCDieState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class NonAtkNPCRespawnState : public FSMState<GameObject>
{
public:
	static NonAtkNPCRespawnState* Instance()
	{
		static NonAtkNPCRespawnState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};
