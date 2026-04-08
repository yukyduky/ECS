#include "stdafx.h"
#include "ComponentManager.h"

#include "WASD/Component/ActionComponent.h"
#include "WASD/Component/AIDebugAlertComponent.h"
#include "WASD/Component/AnimationComponent.h"
#include "WASD/Component/BehaviourComponent.h"
#include "WASD/Component/BehaviourTreeComponent.h"
#include "WASD/Component/CollisionComponent.h"
#include "WASD/Component/DecalComponent.h"
#include "WASD/Component/DirLightComponent.h"
#include "WASD/Component/FlipbookComponent.h"
#include "WASD/Component/HealthComponent.h"
#include "WASD/Component/IdleComponent.h"
#include "WASD/Component/LifetimeComponent.h"
#include "WASD/Component/ModelComponent.h"
#include "WASD/Component/MoveInDirComponent.h"
#include "WASD/Component/MoveOnNavmeshComponent.h"
#include "WASD/Component/PickupComponent.h"
#include "WASD/Component/PlayerDataComponent.h"
#include "WASD/Component/PointLightComponent.h"
#include "WASD/Component/PrefabComponent.h"
#include "WASD/Component/ProjectileCollisionComponent.h"
#include "WASD/Component/RigidBodyComponent.h"
#include "WASD/Component/ScriptComponent.h"
#include "WASD/Component/ShootProjectileComponent.h"
#include "WASD/Component/SoftAreaLightComponent.h"
#include "WASD/Component/SpawnerComponent.h"
#include "WASD/Component/SpotLightComponent.h"
#include "WASD/Component/TextComponent.h"
#include "WASD/Component/TimerComponent.h"
#include "WASD/Component/TransformComponent.h"
#include "WASD/Component/VideoComponent.h"
#include "WASD/Component/WidgetComponent.h"
#include "WASD/Component/UI/UIComponent.h"
#include "WASD/Util/CustomFormatters.h"
#include "WASD/Util/FlagHelpers.h"

#include <limits>
#include <ranges>
#include <magic_enum/magic_enum.hpp>
#include <tge/WASD/Error/Logger.h>
#include <WASD/Component/HealthChangeEventComponent.h>
#include <WASD/Component/TeleportComponent.h>
#include <WASD/Component/Weapon/WeaponContainerComponent.h>

namespace ME = magic_enum;

WASD::ComponentManager::ComponentManager() :
	mySwitchID(GetOrCreateTag("Switch"_tgaid))
{
	RegisterComponentTypes();
	Detail::Type baseType;
	Detail::Archetype* baseArchetype = &myArchetypeIndex[baseType];
	InitializeArchetype(baseArchetype, std::move(baseType));
}

WASD::EntityID WASD::ComponentManager::GetOrCreateTag(const TagID aTag)
{
	if (const auto it = myTags.find(aTag);
		it != myTags.end())
	{
		return it->second;
	}

	const auto entityTagID = static_cast<EntityID>(AddEntity());
	myTags[aTag]           = entityTagID;

	return entityTagID;
}

WASD::EntityID WASD::ComponentManager::GetOrCreateTag(const std::string_view aTag)
{
	const TagID tagID = Tga::StringRegistry::RegisterOrGetString(aTag);
	return GetOrCreateTag(tagID);
}

WASD::EntityID WASD::ComponentManager::GetOrCreateTag(const char* aTag)
{
	const TagID tagID = Tga::StringRegistry::RegisterOrGetString(aTag);
	return GetOrCreateTag(tagID);
}

WASD::UniqueID WASD::ComponentManager::AddEntity()
{
	UniqueID completeID        = 0;
	EntityID id                = 0;
	Detail::GenerationID genID = 1;

	// If dead entities exist then use one of them
	if (myEntityIndex.alive < myEntityIndex.entities.size() && GetQualifierID(myEntityIndex.entities[myEntityIndex.alive]) <
	    std::numeric_limits<Detail::GenerationID>::max())
	{
		completeID = myEntityIndex.entities[myEntityIndex.alive++];
		id         = GetComponentID(completeID);
		genID      = static_cast<Detail::GenerationID>(GetQualifierID(completeID));
		completeID = CombineIDs(id, genID);
	}
	else
	{
		// If a dead one exists, but it's generation/version is maxed then add it to the back so it can be overwritten
		if (myEntityIndex.alive < myEntityIndex.entities.size())
		{
			myEntityIndex.entities.emplace_back(myEntityIndex.entities[myEntityIndex.alive]);
			const EntityID swappedID                     = GetComponentID(myEntityIndex.entities.back());
			myEntityIndex.records[swappedID].entityIndex = static_cast<EntityID>(myEntityIndex.entities.size() - 1);
		}
		else
		{
			myEntityIndex.entities.emplace_back(0);
		}

		constexpr auto componentTypeCount = static_cast<ComponentID>(ME::enum_count<ComponentType>());

		id         = ++myEntityIndex.alive + componentTypeCount;
		completeID = CombineIDs(id, genID);

		myEntityIndex.records[id].entityIndex           = myEntityIndex.alive - 1;
		myEntityIndex.entities[myEntityIndex.alive - 1] = completeID;
	}

	myEntityIndex.records[id].archetype = &myArchetypeIndex[Detail::Type()];
	myEntityIndex.records[id].archetype->entities.emplace_back(completeID);
	myEntityIndex.records[id].row = myEntityIndex.records[id].archetype->entities.size() - 1;

	//LOG_DEBUG("New entity created. Total: {} ID: {} Generation: {}, Complete ID: {}", myEntityIndex.alive, id, GetQualifierID(completeID), completeID);

	return completeID;
}

void WASD::ComponentManager::RemoveEntity(const UniqueID aEntity, const bool aDestroyChildren)
{
	const EntityID id      = GetComponentID(aEntity);
	Detail::Record* record = AccessEntityRecord(aEntity);
	if (!record)
	{
		LOG_WARNING("Tried to remove an invalid Entity '{}'", aEntity);
		return;
	}

	// Entities can exist without components
	if (auto& [archetype, row, entityIndex] = *record;
		archetype)
	{
		const UniqueID idOfLastEntity = archetype->entities.back();

		myEntityIndex.records[GetComponentID(idOfLastEntity)].row = row;
		MoveAndPop(archetype->entities, row);

		FVector entitiesToRemove = MAKE_FVECTOR(EntityID);
		entitiesToRemove.reserve(archetype->type.size());

		for (const UniqueID componentType : archetype->type)
		{
			// ZSC_COMPONENT_COLUMN indicates that it's a tag, relationship, pair, or another entity
			if (const Detail::ColumnIndex mappedColumn = archetype->columnMap.at(componentType);
				mappedColumn != Detail::ZSC_COMPONENT_COLUMN)
			{
				Detail::Column& column        = archetype->components[mappedColumn];
				const ComponentID componentID = GetComponentID(componentType);

				ourComponentTypeData.at(componentID).movePopFunc(column, row);
			}
			else if (aDestroyChildren)
			{
				EntityID entity = GetComponentID(componentType);
				if (Detail::IsFlagSet(componentType, Detail::TypeFlag::Switch))
				{
					entity = GetCase(id, GetQualifierID(componentType));
					RemoveSwitch(id, componentType);
				}

				// If the archetype contains any components then it's an entity with components of its own that needs to be handled
				if (const Detail::Archetype* componentEntityArchetype = myEntityIndex.records[entity].archetype;
					componentEntityArchetype && !componentEntityArchetype->columnMap.empty())
				{
					entitiesToRemove.emplace_back(entity);
				}
			}
		}

		if (aDestroyChildren)
		{
			for (const EntityID entity : entitiesToRemove)
			{
				// TODO: Maybe query the system for any other existing relationships with this entity ID and remove them as well

				RemoveEntity(entity);
			}
		}

		archetype = nullptr;

		--myEntityIndex.alive;
		// ID of entity with last index of alive entities
		const UniqueID swappedID = myEntityIndex.entities[myEntityIndex.alive];
		// Replace removed entity ID with alive entity ID at removed entity index
		myEntityIndex.entities[entityIndex] = swappedID;
		// Update alive entity index
		myEntityIndex.records[GetComponentID(swappedID)].entityIndex = entityIndex;
		// Update dead entity index
		entityIndex = myEntityIndex.alive;

		// Get generation ID of dead entity and increase it by one so that any checks if entity is still alive will return false
		UniqueID newID = GetQualifierID(aEntity);
		newID++;
		newID <<= ENTITY_ID_DIGIT_COUNT;
		newID |= id;
		// Replace alive entity ID with dead entity ID at old alive entity index
		myEntityIndex.entities[entityIndex] = newID;
	}
}

void WASD::ComponentManager::RemoveRelationshipComponent(const UniqueID aEntity, const EntityID aRelatedEntity,
	const RelationshipID aRelationship)
{
	const EntityID relationshipID = GetOrCreateTag(aRelationship);
	const UniqueID combinedID     = CombineIDs(aRelatedEntity, relationshipID);
	RemoveComponent(aEntity, combinedID, {.types = {combinedID}, .count = 1});
}

void WASD::ComponentManager::RemoveComponent(const UniqueID aEntity, const UniqueID aComponentID)
{
	const Detail::Dependencies dependants = GetDependantComponents(aComponentID);
	RemoveComponent(aEntity, aComponentID, dependants);
}

bool WASD::ComponentManager::HasComponent(const UniqueID aEntity, const UniqueID aComponentID)
{
	const Detail::Record* record = GetEntityRecord(aEntity);
	if (!record)
	{
		return false;
	}
	const UniqueID id = Detail::IsFlagSet(aComponentID, Detail::TypeFlag::Switch)
		? Detail::InsertSwitchID(aComponentID, mySwitchID)
		: aComponentID;
	const UniqueID indexID        = id & (Detail::COMPONENT_MASK | Detail::QUALIFIER_MASK);
	const ArchetypeID archetypeID = record->archetype->id;
	return myComponentIndex[indexID][archetypeID].count != 0;
}

FVector<WASD::UniqueID> WASD::ComponentManager::GetTags(UniqueID aEntity) const
{
	FVector<UniqueID> tags;

	const Detail::Record* record = GetEntityRecord(aEntity);
	if (!record)
	{
		LOG_WARNING("Tried to access record of invalid Entity '{}'", aEntity);
		return tags;
	}

	const Detail::Type& archetypeType = record->archetype->type;
	for (const UniqueID componentType : archetypeType)
	{
		if (GetComponentID(componentType) > ME::enum_integer<ComponentType>(ComponentType::Tag))
		{
			tags.emplace_back(componentType);
		}
	}
	return tags;
}

bool WASD::ComponentManager::IsAlive(const UniqueID aEntity) const
{
	if (aEntity == INVALID_ID)
	{
		return false;
	}

	if (const Detail::Record* entityRecord = GetEntityRecord(aEntity);
		!entityRecord)
	{
		return false;
	}

	return true;
}

FVector<WASD::UniqueID> WASD::ComponentManager::GetRelationships(const UniqueID aEntity, const RelationshipID aRelationshipID,
	const Detail::TypeFlag aFlags)
{
	FVector relationships = MAKE_FVECTOR(WASD::UniqueID);

	const Detail::Record* record = GetEntityRecord(aEntity);
	if (!record)
	{
		LOG_WARNING("Tried to get relationships '{}' from an invalid Entity '{}'", aRelationshipID, aEntity);
		return relationships;
	}

	const UniqueID relationshipID = GetOrCreateTag(aRelationshipID);
	const UniqueID searchTerm     = relationshipID << ENTITY_ID_DIGIT_COUNT;

	for (Detail::Archetype* archetype = record->archetype;
	     const UniqueID columnMap : archetype->columnMap | std::views::keys)
	{
		if ((columnMap & searchTerm) == searchTerm)
		{
			relationships.emplace_back(columnMap);
			if (IsFlagSet(aFlags, Detail::TypeFlag::Exclusive))
			{
				break;
			}
		}
	}

	return relationships;
}

WASD::EntityID WASD::ComponentManager::GetCase(const UniqueID aEntity, const EntityID aSwitch) const
{
	return mySwitchLists.at(aSwitch).caseList[Detail::GetCaseIndex(aEntity)].value;
}

const std::unordered_map<WASD::TagID, WASD::EntityID>& WASD::ComponentManager::GetAvailableIDs() const
{
	return myTags;
}

WASD::Detail::Dependencies WASD::ComponentManager::GetDependantComponents(const UniqueID aComponentType)
{
	Detail::Dependencies dependants = {.count = 0};

	if (GetComponentID(aComponentType) > ME::enum_integer(ComponentType::Tag))
	{
		dependants.types[dependants.count++] = aComponentType;
		return dependants;
	}

	auto checkIfDependant = [&](const Detail::Dependencies& aDependencies)
	{
		const auto begin = aDependencies.types.cbegin();
		const auto end   = aDependencies.types.cbegin() + aDependencies.count;

		return std::ranges::any_of(begin, end, [&](const UniqueID aComponentID)
		{
			return aComponentType == aComponentID;
		});
	};

	for (const Detail::ComponentTypeData& typeData : ourComponentTypeData | std::views::values)
	{
		if (checkIfDependant(typeData.dependencies))
		{
			dependants.types[dependants.count++] = typeData.id;
		}
	}

	dependants.types[dependants.count++] = aComponentType;

	return dependants;
}

WASD::UniqueID WASD::ComponentManager::GetFullID(const EntityID aPartialID) const
{
	const Detail::Record* record = GetEntityRecord(aPartialID);
	if (!record)
	{
		LOG_WARNING("Tried to get the full ID of partial entity ID '{}'", aPartialID);
		return INVALID_ID;
	}
	return myEntityIndex.entities[record->entityIndex];
}

void WASD::ComponentManager::RemoveComponent(const UniqueID aEntity, const UniqueID aComponentID, const Detail::Dependencies& aDependants)
{
	Detail::Record* record = AccessEntityRecord(aEntity);
	if (!record)
	{
		LOG_WARNING("Tried to remove component of type '{}' with ID '{}' from an invalid Entity '{}'", GetComponentID(aComponentID),
		            aComponentID, aEntity);
		return;
	}

	const bool isSwitch        = Detail::IsFlagSet(aComponentID, Detail::TypeFlag::Switch);
	const UniqueID componentID = isSwitch ? Detail::InsertSwitchID(aComponentID, mySwitchID) : aComponentID;

	if (const std::size_t componentCount = CalcComponentCount(*record, componentID);
		componentCount == 0)
	{
		return;
	}

	Detail::Archetype* archetype     = record->archetype;
	Detail::Archetype* nextArchetype = archetype->edges[componentID].remove;

	if (!nextArchetype)
	{
		Detail::Type newType = RemoveTypesFromArchetypeType(archetype->type, aDependants);
		InitializeArchetype(nextArchetype, std::move(newType));
	}

	if (const Detail::ColumnIndex columnIndex = archetype->columnMap.at(componentID);
		columnIndex != Detail::ZSC_COMPONENT_COLUMN)
	{
		const ComponentID id   = GetComponentID(componentID);
		Detail::Column& column = archetype->components[columnIndex];

		ourComponentTypeData.at(id).movePopFunc(column, record->row);
	}
	else if (isSwitch)
	{
		RemoveSwitch(aEntity, aComponentID);
	}

	MoveEntity(record, nextArchetype);
}

std::size_t WASD::ComponentManager::CalcComponentCount(const Detail::Record& aEntityRecord, const UniqueID aComponentID)
{
#ifndef _RETAIL
	if (!aEntityRecord.archetype)
	{
		LOG_WARNING("Entity '{}' archetype was invalid", myEntityIndex.entities[aEntityRecord.entityIndex]);
		return 0;
	}
#endif
	const ArchetypeID archetypeID = aEntityRecord.archetype->id;
	const UniqueID indexID        = aComponentID & (Detail::COMPONENT_MASK | Detail::QUALIFIER_MASK);
	return myComponentIndex[indexID][archetypeID].count;
}

const WASD::Detail::Record* WASD::ComponentManager::GetEntityRecord(const UniqueID aEntity) const
{
	const auto it = myEntityIndex.records.find(GetComponentID(aEntity));
	if (it == myEntityIndex.records.end())
	{
		return nullptr;
	}

	const Detail::Record& record = it->second;
	if (record.entityIndex >= myEntityIndex.alive)
	{
		return nullptr;
	}

	if (myEntityIndex.entities[record.entityIndex] != aEntity && GetQualifierID(aEntity) != 0)
	{
		return nullptr;
	}

	return &record;
}

WASD::Detail::Record* WASD::ComponentManager::AccessEntityRecord(const UniqueID aEntity)
{
	const auto it = myEntityIndex.records.find(GetComponentID(aEntity));
	if (it == myEntityIndex.records.end())
	{
		return nullptr;
	}

	const Detail::Record& record = it->second;
	if (record.entityIndex >= myEntityIndex.alive)
	{
		return nullptr;
	}

	if (myEntityIndex.entities[record.entityIndex] != aEntity && GetQualifierID(aEntity) != 0)
	{
		return nullptr;
	}

	return &it->second;
}

void WASD::ComponentManager::InitializeArchetype(Detail::Archetype*& aNewArchetype, Detail::Type&& aType)
{
	aNewArchetype = &myArchetypeIndex[aType];

	if (aNewArchetype->id == Detail::INVALID_ARCHETYPE_ID)
	{
		const std::size_t typeCount = aType.size();

		aNewArchetype->id   = myArchetypeIDs++;
		aNewArchetype->type = std::move(aType);

		aNewArchetype->columnMap.reserve(typeCount);

		auto initRecord = [&aNewArchetype](Detail::ArchetypeRecord& aRecord)
		{
			aRecord.archetype = aNewArchetype;
			aRecord.count++;
		};

		std::size_t columnIndex = 0;
		for (const UniqueID componentType : aNewArchetype->type)
		{
			const UniqueID indexID = componentType & (Detail::COMPONENT_MASK | Detail::QUALIFIER_MASK);
			initRecord(myComponentIndex[indexID][aNewArchetype->id]);

			if (componentType != 0)
			{
				// Added to be able to query using wildcards
				// Add wildcard for id part
				if (const UniqueID targetID = componentType & Detail::COMPONENT_MASK;
					targetID != componentType && targetID != 0)
				{
					initRecord(myComponentIndex[targetID][aNewArchetype->id]);
				}
				// Add wildcard for qualifier part
				if (const UniqueID qualifierMask = componentType & Detail::QUALIFIER_MASK;
					qualifierMask != componentType && qualifierMask != 0)
				{
					initRecord(myComponentIndex[qualifierMask][aNewArchetype->id]);
				}
				// Add wildcard for both
				initRecord(myComponentIndex[0][aNewArchetype->id]);

				if (const UniqueID componentID = GetComponentID(componentType);
					componentID < static_cast<unsigned>(ComponentType::Tag))
				{
					aNewArchetype->columnMap[componentType] = columnIndex++;
				}
				else
				{
					aNewArchetype->columnMap[componentType] = Detail::ZSC_COMPONENT_COLUMN;
				}
			}
		}

		// We only store the ones that are not mapped to ZSC_COMPONENT_COLUMN
		aNewArchetype->components.reserve(columnIndex);
		for (const UniqueID id : aNewArchetype->type)
		{
			if (const ComponentID componentID = GetComponentID(id);
				componentID < ME::enum_integer<ComponentType>(ComponentType::Tag) && componentID != 0)
			{
				aNewArchetype->components.emplace_back(ourComponentTypeData.at(componentID).size);
			}
		}
	}
}

void WASD::ComponentManager::MoveEntity(Detail::Record*& aRecord, Detail::Archetype*& aNextArchetype)
{
	// If it's a new entity then there won't be any components to move
	ArchetypeEntities& currentArchetypeEntities = aRecord->archetype->entities;
	const EntityID idOfLastEntity               = GetComponentID(currentArchetypeEntities.back());

	Detail::Record* record         = AccessEntityRecord(idOfLastEntity);
	Detail::RowID& rowOfLastEntity = record->row;

	const Detail::Type& type        = aRecord->archetype->type;
	const Detail::Type& nextType    = aNextArchetype->type;
	const std::size_t typeCount     = type.size();
	const std::size_t nextTypeCount = nextType.size();
	const std::size_t minType       = std::ranges::min(typeCount, nextTypeCount);

	std::size_t i     = 0;
	std::size_t iNext = 0;
	for (std::size_t count = 0; count < minType; ++count)
	{
		while (i < typeCount && iNext < nextTypeCount && nextType[iNext] != type[i])
		{
			nextType[iNext] < type[i] ? iNext++ : i++;
		}

		if (i == typeCount || iNext == nextTypeCount)
		{
			break;
		}

		const UniqueID componentID = type[i];
		if (const Detail::ColumnIndex columnIndex = aRecord->archetype->columnMap[componentID];
			columnIndex != Detail::ZSC_COMPONENT_COLUMN)
		{
			if (const UniqueID nextComponentID = nextType[iNext];
				componentID == nextComponentID)
			{
				const Detail::ColumnIndex nextColumnIndex = aNextArchetype->columnMap.at(nextComponentID);
				Detail::Column& fromColumn                = aRecord->archetype->components[columnIndex];
				Detail::Column& toColumn                  = aNextArchetype->components[nextColumnIndex];
				const Detail::RowID row                   = aRecord->row;
				const ComponentID id                      = GetComponentID(nextComponentID);

				// Move the component
				ourComponentTypeData.at(id).transferFunc(toColumn, fromColumn, row);

				i++;
				iNext++;
			}
		}
	}

	const Detail::RowID row = aRecord->row;
	const UniqueID entity   = currentArchetypeEntities[row];

	// Move the entity
	aNextArchetype->entities.emplace_back(entity);
	MoveAndPop(currentArchetypeEntities, row);
	rowOfLastEntity = aRecord->row;

	aRecord->archetype = aNextArchetype;
	aRecord->row       = aNextArchetype->entities.size() - 1;
}

WASD::Detail::Type WASD::ComponentManager::AddTypesToArchetypeType(const Detail::Type& aCurrentArchetypeType,
	Detail::Dependencies& aComponentTypes)
{
	if (aComponentTypes.count > 1)
	{
		const auto begin = aComponentTypes.types.begin();
		const auto end   = aComponentTypes.types.begin() + aComponentTypes.count;

		for (auto it = begin; it != begin + aComponentTypes.count;)
		{
			const bool removeType = std::ranges::any_of(aCurrentArchetypeType, [&](const UniqueID aArchetypeType)
			{
				return *it == aArchetypeType;
			});

			if (removeType)
			{
				if (it + 1 != end)
				{
					std::ranges::copy(it + 1, end, it);
				}
				--aComponentTypes.count;
			}
			else
			{
				++it;
			}
		}
	}

	Detail::Type newType;
	newType.reserve(aCurrentArchetypeType.size() + aComponentTypes.count);
	newType = aCurrentArchetypeType;

	for (unsigned i = 0; i < aComponentTypes.count; ++i)
	{
		newType.emplace_back(aComponentTypes.types[i]);
	}

	std::ranges::sort(newType);
	return newType;
}

WASD::Detail::Type WASD::ComponentManager::RemoveTypesFromArchetypeType(const Detail::Type& aCurrentArchetypeType,
	const Detail::Dependencies& aComponentTypes)
{
	Detail::Type newType;
	const std::size_t typeSize = aCurrentArchetypeType.size() < aComponentTypes.count
		? 0
		: aCurrentArchetypeType.size() - aComponentTypes.count;
	newType.reserve(typeSize);

	for (const UniqueID currentArchetypeType : aCurrentArchetypeType)
	{
		const auto begin = aComponentTypes.types.cbegin();
		const auto end   = aComponentTypes.types.cbegin() + aComponentTypes.count;

		if (std::ranges::all_of(begin, end, [currentArchetypeType](const UniqueID aComponentType)
		{
			return currentArchetypeType != aComponentType;
		}))
		{
			newType.emplace_back(currentArchetypeType);
		}
	}
	return newType;
}

void WASD::ComponentManager::RemoveSwitch(const UniqueID aEntity, const UniqueID aSwitchComponent)
{
	const Detail::IndexSize caseIndex = Detail::GetCaseIndex(aEntity);
	const EntityID switchIndex        = GetQualifierID(aSwitchComponent);
	Detail::SwitchList& switchList    = mySwitchLists[switchIndex];

	Detail::CaseNode* caseNode = &switchList.caseList[caseIndex];

	if (caseNode->nextIndex != 0)
	{
		switchList.caseList[caseNode->nextIndex].prevIndex = caseNode->prevIndex;
	}

	const EntityID caseValue = switchList.caseList[caseIndex].value;
	if (const Detail::IndexSize headIndex = switchList.headCaseIndex.at(caseValue);
		caseIndex == headIndex)
	{
		switchList.headCaseIndex.at(caseValue) = caseNode->nextIndex;
	}
	else if (caseNode->prevIndex != 0)
	{
		switchList.caseList[caseNode->prevIndex].nextIndex = caseNode->nextIndex;
		caseNode->prevIndex                                = 0;
	}
	caseNode->nextIndex = 0;
}

void WASD::ComponentManager::RegisterComponentTypes()
{
	ourComponentTypeData[TransformComponent::typeID] = TransformComponent::GetComponentTypeData();
}
