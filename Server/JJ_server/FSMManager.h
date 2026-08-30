#pragma once
#include "FSMState.h"
#include <memory>
template <class entity_type>
class FSMManager
{
	std::weak_ptr<entity_type> Owner;
	// 공유 싱글턴 상태를 가리키기만 한다(비소유). 전이 시 포인터 대입만 발생.
	FSMState<entity_type>* Currentstate = nullptr;
	FSMState<entity_type>* Globalstate = nullptr;

public:
	FSMManager(std::shared_ptr<entity_type> owner)
		: Owner(owner) {}
	virtual ~FSMManager() {}

	FSMManager(const FSMManager&) = delete;
	FSMManager& operator=(const FSMManager&) = delete;

	void SetCurrentState(FSMState<entity_type>* s)
	{
		Currentstate = s;
	}
	void SetGlobalState(FSMState<entity_type>* s)
	{
		Globalstate = s;
	}

	void Update() const
	{
		if (auto owner_sp = Owner.lock()) {
			if (Globalstate) Globalstate->Execute(owner_sp);
			if (Currentstate) Currentstate->Execute(owner_sp);
		}
	}

	void ChangeState(FSMState<entity_type>* newstate)
	{
		if (auto owner_sp = Owner.lock()) {
			if (Currentstate) Currentstate->Exit(owner_sp);
			Currentstate = newstate;
			Currentstate->Enter(owner_sp);
		}
	}

	FSMState<entity_type>* GetCurrentState() const
	{
		return Currentstate;
	}
};
