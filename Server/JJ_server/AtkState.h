#pragma once
#include "FSMState.h"
class GameObject;

// 공격형 NPC(늑대/거미/두꺼비/박쥐/랩터) 상태.
// 각 상태는 인스턴스 1개(Instance)만 공유하며 데이터 멤버가 없다.
// 타이머/이동타입 등 객체별 값은 GameObject::m_fsmCtx 사용.

class AtkNPCGlobalState : public FSMState<GameObject>
{
public:
	static AtkNPCGlobalState* Instance()
	{
		static AtkNPCGlobalState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class AtkNPCStandingState : public FSMState<GameObject>
{
public:
	static AtkNPCStandingState* Instance()
	{
		static AtkNPCStandingState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class AtkNPCMoveState : public FSMState<GameObject>
{
public:
	static AtkNPCMoveState* Instance()
	{
		static AtkNPCMoveState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class AtkNPCChaseState : public FSMState<GameObject>
{
public:
	static AtkNPCChaseState* Instance()
	{
		static AtkNPCChaseState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class AtkNPCAttackState : public FSMState<GameObject>
{
public:
	static AtkNPCAttackState* Instance()
	{
		static AtkNPCAttackState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class AtkNPCDieState : public FSMState<GameObject>
{
public:
	static AtkNPCDieState* Instance()
	{
		static AtkNPCDieState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class AtkNPCRespawnState : public FSMState<GameObject>
{
public:
	static AtkNPCRespawnState* Instance()
	{
		static AtkNPCRespawnState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class AtkNPCHitState : public FSMState<GameObject>
{
public:
	static AtkNPCHitState* Instance()
	{
		static AtkNPCHitState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};
