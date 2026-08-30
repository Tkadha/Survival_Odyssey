#pragma once
#include <memory>

// 상태 클래스는 종류마다 인스턴스 1개만 두고(싱글턴) 여러 객체가 공유한다.
// 따라서 상태 클래스는 가변 멤버를 가지면 안 되며, 객체별 데이터는
// 인자로 받은 entity(GameObject::m_fsmCtx)에 저장/조회한다.
template <class entity_type>
class FSMState
{
public:
	virtual ~FSMState() {}

	virtual void Enter(const std::shared_ptr<entity_type>& npc) = 0;
	virtual void Execute(const std::shared_ptr<entity_type>& npc) = 0;
	virtual void Exit(const std::shared_ptr<entity_type>& npc) = 0;
};
