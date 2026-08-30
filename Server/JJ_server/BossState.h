#pragma once
#include "FSMState.h"
class GameObject;

// 보스(골렘) 상태.
// 각 상태는 인스턴스 1개(Instance)만 공유하며 데이터 멤버가 없다.
// 타이머/어그로 대상/특수공격 카운터 등 객체별 값은 GameObject::m_fsmCtx 사용.

class BossGlobalState : public FSMState<GameObject>
{
public:
	static BossGlobalState* Instance()
	{
		static BossGlobalState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class BossStandingState : public FSMState<GameObject>
{
public:
	static BossStandingState* Instance()
	{
		static BossStandingState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class BossMoveState : public FSMState<GameObject>
{
public:
	static BossMoveState* Instance()
	{
		static BossMoveState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class BossChaseState : public FSMState<GameObject>
{
public:
	static BossChaseState* Instance()
	{
		static BossChaseState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class BossAttackState : public FSMState<GameObject>
{
public:
	static BossAttackState* Instance()
	{
		static BossAttackState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class BossDieState : public FSMState<GameObject>
{
public:
	static BossDieState* Instance()
	{
		static BossDieState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class BossRespawnState : public FSMState<GameObject>
{
public:
	static BossRespawnState* Instance()
	{
		static BossRespawnState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class BossHitState : public FSMState<GameObject>
{
public:
	static BossHitState* Instance()
	{
		static BossHitState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class BossSpecialAttackStartState : public FSMState<GameObject>
{
public:
	static BossSpecialAttackStartState* Instance()
	{
		static BossSpecialAttackStartState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};

class BossSpecialAttackEndState : public FSMState<GameObject>
{
public:
	static BossSpecialAttackEndState* Instance()
	{
		static BossSpecialAttackEndState s;
		return &s;
	}
	void Enter(const std::shared_ptr<GameObject>& npc) override;
	void Execute(const std::shared_ptr<GameObject>& npc) override;
	void Exit(const std::shared_ptr<GameObject>& npc) override;
};
