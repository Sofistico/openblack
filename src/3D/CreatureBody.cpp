/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureBody.h"

#include <map>
#include <string>
#include <vector>

#include <entt/entt.hpp>
#include <spdlog/spdlog.h>

#include "Common/StringUtils.h"
#include "Enums.h"

using namespace openblack;
using namespace openblack::creature;

using S = CreatureType;
const std::map<std::string, CreatureType> k_MeshNameToSpecies = {{"c_cow", S::Cow},
                                                                 {"a_tiger2", S::Tiger},
                                                                 {"c_leopard", S::Leopard},
                                                                 {"c_wolf", S::Wolf},
                                                                 {"c_lion", S::Lion},
                                                                 {"a_horse", S::Horse},
                                                                 {"c_tortoise", S::Tortoise},
                                                                 {"c_zebra", S::Zebra},
                                                                 {"a_bear_boned", S::BrownBear},
                                                                 {"c_polar_bear", S::PolarBear},
                                                                 {"c_sheep", S::Sheep},
                                                                 {"c_chimp", S::Chimp},
                                                                 {"a_greek_boned", S::Ogre},
                                                                 {"c_mandrill_boned", S::Mandrill},
                                                                 {"c_rhino", S::Rhino},
                                                                 {"c_gorilla_boned", S::Gorilla},
                                                                 {"c_ape_boned", S::GiantApe}};

using A = CreatureBody::Appearance;
const std::map<std::string, CreatureBody::Appearance> k_MeshNameToAppearance = {
    {"boned", A::Base}, {"base", A::Base},     {"good2", A::Good}, {"base2", A::Base}, {"good", A::Good}, {"evil", A::Evil},
    {"evil2", A::Evil}, {"strong", A::Strong}, {"weak", A::Weak},  {"fat", A::Fat},    {"thin", A::Thin}};

entt::id_type creature::GetIdFromType(CreatureType species, CreatureBody::Appearance appearance)
{
	return entt::hashed_string(
	    fmt::format("creature/{}/{}", static_cast<uint8_t>(species), static_cast<uint32_t>(appearance)).c_str());
}

entt::id_type creature::GetIdFromMeshName(const std::string& name)
{
	auto species = S::Unknown;
	auto appearance = A::Base;
	auto split = string_utils::Split(string_utils::LowerCase(name), "_");
	auto suffix = split[split.size() - 1];

	auto appearanceFound = k_MeshNameToAppearance.find(suffix);
	if (appearanceFound == k_MeshNameToAppearance.end())
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unknown creature appearance: {}", name);
	}
	else
	{
		appearance = appearanceFound->second;
	}

	// Remove the suffix and find the creature species
	split.erase(split.begin() + split.size() - 1);
	auto speciesFound = k_MeshNameToSpecies.find(fmt::format("{}", fmt::join(split, "_")));
	if (speciesFound == k_MeshNameToSpecies.end())
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unknown creature species: {}", name);
	}
	else
	{
		species = speciesFound->second;
	}

	return entt::hashed_string(
	    fmt::format("creature/{}/{}", static_cast<uint8_t>(species), static_cast<uint32_t>(appearance)).c_str());
}
