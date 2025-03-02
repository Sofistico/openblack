/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Components/LivingAction.h"

namespace openblack::ecs::systems
{

class LivingActionSystemInterface
{
public:
	virtual void Update() = 0;

	[[nodiscard]] virtual VillagerStates VillagerGetState(const components::LivingAction& action,
	                                                      components::LivingAction::Index index) const = 0;
	virtual void VillagerSetState(components::LivingAction& action, components::LivingAction::Index index, VillagerStates state,
	                              bool skipTransition) const = 0;
	virtual uint32_t VillagerCallState(components::LivingAction& action, components::LivingAction::Index index) const = 0;
	virtual bool VillagerCallEntryState(components::LivingAction& action, components::LivingAction::Index index,
	                                    VillagerStates src, VillagerStates dst) const = 0;
	virtual bool VillagerCallExitState(components::LivingAction& action, components::LivingAction::Index index) const = 0;
	virtual int VillagerCallOutOfAnimation(components::LivingAction& action, components::LivingAction::Index index) const = 0;
	virtual bool VillagerCallValidate(components::LivingAction& action, components::LivingAction::Index index) const = 0;
};

} // namespace openblack::ecs::systems
