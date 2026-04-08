#pragma once
#include "WASD/Component/ComponentInfo.h"
#include "WASD/Component/TagComponent.h"
#include "WASD/Input/InputInfo.h"

#include <unordered_map>
#include <vector>
#include <tge/WASD/Container/TypelessVector.hpp>
#include <tge/WASD/Memory/frame_containers.h>

namespace WASD
{
	class ComponentManager
	{
	public:
		ComponentManager();
		ComponentManager(ComponentManager&& aOther) noexcept            = delete;
		ComponentManager(ComponentManager& aOther)                      = delete;
		ComponentManager& operator=(ComponentManager&& aOther) noexcept = delete;
		ComponentManager& operator=(const ComponentManager& aOther)     = delete;
		~ComponentManager()                                             = default;

		EntityID GetOrCreateTag(TagID aTag);
		EntityID GetOrCreateTag(std::string_view aTag);
		EntityID GetOrCreateTag(const char* aTag);

		[[nodiscard]] UniqueID AddEntity();
		void RemoveEntity(UniqueID aEntity, bool aDestroyChildren = true);

		template<Detail::isComponent Derived, typename... Args>
		Derived* AccessOrAddComponentWithID(UniqueID aEntity, UniqueID aComponentID, Args&&... aArgs);
		template<Detail::isComponent Derived, typename... Args> requires (!std::is_same_v<Derived, TagComponent>)
		Derived* AccessOrAddComponent(UniqueID aEntity, Args&&... aArgs);

		void RemoveRelationshipComponent(UniqueID aEntity, EntityID aRelatedEntity, RelationshipID aRelationship);
		[[nodiscard]] FVector<UniqueID> GetRelationships(UniqueID aEntity, RelationshipID aRelationshipID, Detail::TypeFlag aFlags = {});

		template<Detail::isComponent Derived> requires (!std::is_same_v<Derived, TagComponent>)
		void RemoveComponent(UniqueID aEntity);
		void RemoveComponent(UniqueID aEntity, UniqueID aComponentID);

		template<Detail::isComponent Derived>
		[[nodiscard]] bool HasComponent(UniqueID aEntity);
		[[nodiscard]] bool HasComponent(UniqueID aEntity, UniqueID aComponentID);
		[[nodiscard]] FVector<UniqueID> GetTags(UniqueID aEntity) const;

		[[nodiscard]] bool IsAlive(UniqueID aEntity) const;

		template<typename... Qs, std::size_t N = sizeof...(Qs)> requires ((std::is_convertible_v<Qs, QVariable>) && ...)
		Entities Query(const QVariable& aSource, const Qs&... aQueries);

		template<Detail::isComponent Derived>
		[[nodiscard]] Derived* AccessComponent(UniqueID aEntity);
		template<Detail::isComponent Derived> requires (!std::is_same_v<Derived, TagComponent>)
		[[nodiscard]] Derived* AccessComponent(UniqueID aEntity, UniqueID aComponentID);
		[[nodiscard]] EntityID GetCase(UniqueID aEntity, EntityID aSwitch) const;

		[[nodiscard]] const std::unordered_map<TagID, EntityID>& GetAvailableIDs() const;
		static Detail::Dependencies GetDependantComponents(UniqueID aComponentType);

		UniqueID GetFullID(EntityID aPartialID) const;

	private:
		void RemoveComponent(UniqueID aEntity, UniqueID aComponentID, const Detail::Dependencies& aDependants);

		[[nodiscard]] std::size_t CalcComponentCount(const Detail::Record& aEntityRecord, UniqueID aComponentID);
		[[nodiscard]] const Detail::Record* GetEntityRecord(UniqueID aEntity) const;
		[[nodiscard]] Detail::Record* AccessEntityRecord(UniqueID aEntity);
		void InitializeArchetype(Detail::Archetype*& aNewArchetype, Detail::Type&& aType);
		void MoveEntity(Detail::Record*& aRecord, Detail::Archetype*& aNextArchetype);

		[[nodiscard]] static Detail::Type AddTypesToArchetypeType(const Detail::Type& aCurrentArchetypeType,
			Detail::Dependencies& aComponentTypes);
		[[nodiscard]] static Detail::Type RemoveTypesFromArchetypeType(const Detail::Type& aCurrentArchetypeType,
			const Detail::Dependencies& aComponentTypes);
		void RemoveSwitch(UniqueID aEntity, UniqueID aSwitchComponent);
		static void RegisterComponentTypes();

		Detail::EntityIndex myEntityIndex;
		std::unordered_map<UniqueID, Detail::ArchetypeMap> myComponentIndex;
		std::unordered_map<Detail::Type, Detail::Archetype, Detail::TypeHasher> myArchetypeIndex;
		ArchetypeID myArchetypeIDs = 0;
		std::unordered_map<TagID, EntityID> myTags;
		CacheMap<EntityID, Detail::SwitchList> mySwitchLists;
		UniqueID mySwitchID = INVALID_ID;

		inline static std::unordered_map<ComponentID, Detail::ComponentTypeData> ourComponentTypeData;
	};

	template<Detail::isComponent Derived, typename... Args>
	Derived* ComponentManager::AccessOrAddComponentWithID(const UniqueID aEntity, const UniqueID aComponentID, Args&&... aArgs)
	{
		Detail::Record* record = AccessEntityRecord(aEntity);
		if (!record)
		{
			LOG_WARNING("Tried to add component of type '{}' with ID '{}' to an invalid Entity '{}'", Derived::typeString, aComponentID,
			            aEntity);
			return nullptr;
		}

		Detail::Archetype* archetype = record->archetype;

		const bool isSwitch        = Detail::IsFlagSet(aComponentID, Detail::TypeFlag::Switch);
		const UniqueID componentID = isSwitch ? Detail::InsertSwitchID(aComponentID, mySwitchID) : aComponentID;

		const std::size_t componentCount = CalcComponentCount(*record, componentID);
		if (componentCount != 0)
		{
			if (!isSwitch)
			{
				if (const Detail::ColumnIndex columnIndex = archetype->columnMap.at(componentID);
					columnIndex != Detail::ZSC_COMPONENT_COLUMN)
				{
					return &archetype->components[columnIndex].at<Derived>(record->row);
				}
				return nullptr;
			}
		}

		if (isSwitch)
		{
			[this, aEntity, aComponentID, componentCount]
			{
				const Detail::IndexSize caseIndex = Detail::GetCaseIndex(aEntity);
				const EntityID switchIndex        = GetQualifierID(aComponentID);
				Detail::SwitchList& switchList    = mySwitchLists[switchIndex];
				const EntityID caseValue          = GetComponentID(aComponentID);

				// Resize based on largest entity that uses this switch
				if (switchList.caseList.size() <= caseIndex)
				{
					switchList.caseList.resize(caseIndex + 1);
				}
				else if (caseValue == switchList.caseList[caseIndex].value)
				{
					return;
				}

				if (componentCount != 0)
				{
					if (const EntityID oldValue = switchList.caseList[caseIndex].value;
						switchList.headCaseIndex.contains(oldValue))
					{
						Detail::CaseNode* caseNode = &switchList.caseList[caseIndex];

						if (caseNode->nextIndex != 0)
						{
							switchList.caseList[caseNode->nextIndex].prevIndex = caseNode->prevIndex;
						}

						if (const Detail::IndexSize headIndex = switchList.headCaseIndex.at(oldValue);
							caseIndex == headIndex)
						{
							switchList.headCaseIndex.at(oldValue) = caseNode->nextIndex;
						}
						else if (caseNode->prevIndex != 0 /*|| caseNode->prevIndex == headIndex*/)
						{
							switchList.caseList[caseNode->prevIndex].nextIndex = caseNode->nextIndex;
						}

						caseNode->nextIndex = 0;
						caseNode->prevIndex = 0;
					}
				}

				switchList.caseList[caseIndex].value = caseValue;
				if (!switchList.headCaseIndex.contains(caseValue) || switchList.headCaseIndex.at(caseValue) == 0)
				{
					switchList.headCaseIndex[caseValue] = caseIndex;
					return;
				}

				const Detail::IndexSize headIndex = switchList.headCaseIndex.at(caseValue);
				const Detail::CaseNode* caseNode  = &switchList.caseList[headIndex];

				// Check if instance should be new head case
				if (caseNode > &switchList.caseList[caseIndex])
				{
					switchList.headCaseIndex.at(caseValue)   = caseIndex;
					switchList.caseList[caseIndex].nextIndex = headIndex;
					switchList.caseList[caseIndex].prevIndex = 0;
					switchList.caseList[headIndex].prevIndex = caseIndex;
					return;
				}

				// Find the instance before in the interleaved linked list
				Detail::IndexSize nextIndex = caseNode->nextIndex;
				while (nextIndex != 0 && nextIndex < caseIndex)
				{
					caseNode  = &switchList.caseList[nextIndex];
					nextIndex = caseNode->nextIndex;
				}
				const Detail::IndexSize prevIndex = caseNode->prevIndex == 0
					? headIndex
					: switchList.caseList[caseNode->prevIndex].nextIndex;

				if (nextIndex != 0)
				{
					switchList.caseList[caseIndex].nextIndex = nextIndex;
					switchList.caseList[nextIndex].prevIndex = caseIndex;
				}
				switchList.caseList[caseIndex].prevIndex = prevIndex;
				switchList.caseList[prevIndex].nextIndex = caseIndex;
			}();
		}

		Detail::Archetype* nextArchetype = archetype->edges[componentID].add;

		Detail::Dependencies dependencies = Derived::dependencies;

		if (!nextArchetype)
		{
			dependencies.types[dependencies.count++] = componentID;
			Detail::Type newType                     = AddTypesToArchetypeType(archetype->type, dependencies);
			InitializeArchetype(nextArchetype, std::move(newType));
			--dependencies.count;
		}

		if (dependencies.count != 0)
		{
			for (unsigned i = 0; i < dependencies.count; ++i)
			{
				const ComponentID id                  = GetComponentID(dependencies.types[i]);
				const Detail::ColumnIndex columnIndex = nextArchetype->columnMap.at(dependencies.types[i]);
				ourComponentTypeData.at(id).constructFunc(nextArchetype->components[columnIndex]);
			}
		}

		Derived* component = nullptr;

		if (const Detail::ColumnIndex columnIndex = nextArchetype->columnMap.at(componentID);
			columnIndex != Detail::ZSC_COMPONENT_COLUMN)
		{
			component = &nextArchetype->components[columnIndex].emplace_back<Derived>(std::forward<Args>(aArgs)...);
		}

		MoveEntity(record, nextArchetype);

		return component;
	}

	template<Detail::isComponent Derived, typename... Args> requires (!std::is_same_v<Derived, TagComponent>)
	Derived* ComponentManager::AccessOrAddComponent(const UniqueID aEntity, Args&&... aArgs)
	{
		return AccessOrAddComponentWithID<Derived>(aEntity, static_cast<UniqueID>(Derived::typeID), std::forward<Args>(aArgs)...);
	}

	template<Detail::isComponent Derived> requires (!std::is_same_v<Derived, TagComponent>)
	void ComponentManager::RemoveComponent(const UniqueID aEntity)
	{
		const Detail::Dependencies dependants = GetDependantComponents(Derived::typeID);
		RemoveComponent(aEntity, Derived::typeID, dependants);
	}

	template<Detail::isComponent Derived>
	bool ComponentManager::HasComponent(const UniqueID aEntity)
	{
		constexpr UniqueID componentID = Derived::typeID;
		return HasComponent(aEntity, componentID);
	}

	template<typename... Qs, std::size_t N> requires ((std::is_convertible_v<Qs, QVariable>) && ...)
	Entities ComponentManager::Query(const QVariable& aSource, const Qs&... aQueries)
	{
		const std::array queries = to_array<QVariable>(aQueries...);

		Entities queriedEntities = MAKE_FVECTOR(UniqueID);

		auto resolveTerm = [this](const QVariable& aVariable, Detail::Archetype& aArchetype)
		{
			Detail::Archetype* nextArchetype = &aArchetype;

			if (aVariable)
			{
				if (myComponentIndex[aVariable.term.value][aArchetype.id].count != 0)
				{
					return nextArchetype;
				}

				auto& [add, remove] = aArchetype.edges[aVariable.term.value];
				nextArchetype       = add;

				if (!nextArchetype)
				{
					const ComponentID componentID = GetComponentID(aVariable.term.value);
					Detail::Dependencies dependencies;

					if (componentID < TagComponent::typeID && componentID != 0)
					{
						dependencies = ourComponentTypeData.at(componentID).dependencies;
					}
					dependencies.types[dependencies.count++] = aVariable.term.value;

					Detail::Type newType = AddTypesToArchetypeType(aArchetype.type, dependencies);
					InitializeArchetype(nextArchetype, std::move(newType));
					--dependencies.count;
				}

				return nextArchetype;
			}

			if (myComponentIndex[aVariable.term.value][aArchetype.id].count == 0)
			{
				return nextArchetype;
			}

			auto& [add, remove] = aArchetype.edges[aVariable.term.value];
			nextArchetype       = remove;

			if (!nextArchetype)
			{
				const ComponentID componentID = GetComponentID(aVariable.term.value);
				Detail::Dependencies dependants;

				if (componentID < TagComponent::typeID && componentID != 0)
				{
					dependants = ourComponentTypeData.at(componentID).dependants;
				}
				dependants.types[dependants.count++] = aVariable.term.value;

				Detail::Type newType = RemoveTypesFromArchetypeType(aArchetype.type, dependants);
				InitializeArchetype(nextArchetype, std::move(newType));
				--dependants.count;
			}

			return nextArchetype;
		};

		const UniqueID source = aSource.source == 0 ? aSource.term.value : aSource.source;

		FUnorderedSet<ArchetypeID> queriedArchetypes = MAKE_FUNORDERED_SET(ArchetypeID);
		for (const auto& [archetype, count] : myComponentIndex[source] | std::views::values)
		{
			if (count != 0)
			{
				Detail::Archetype* currentArchetype = archetype;
				if (!currentArchetype->entities.empty())
				{
					for (unsigned i = 0; i < N; ++i)
					{
						currentArchetype = resolveTerm(queries[i], *currentArchetype);
					}
					if (const auto [it, wasInserted] = queriedArchetypes.insert(currentArchetype->id);
						wasInserted)
					{
						queriedEntities.insert(queriedEntities.end(), currentArchetype->entities.begin(), currentArchetype->entities.end());
					}
				}
			}
		}

		return queriedEntities;
	}

	template<Detail::isComponent Derived>
	Derived* ComponentManager::AccessComponent(const UniqueID aEntity)
	{
		return AccessComponent<Derived>(aEntity, Derived::typeID);
	}

	template<Detail::isComponent Derived> requires (!std::is_same_v<Derived, TagComponent>)
	Derived* ComponentManager::AccessComponent(const UniqueID aEntity, const UniqueID aComponentID)
	{
		const Detail::Record* record = GetEntityRecord(aEntity);
		if (!record)
		{
			LOG_WARNING("Tried to get component type '{}' with ID '{}' from an invalid Entity '{}'", Derived::typeString, aComponentID,
			            aEntity);
			return nullptr;
		}
		if (const std::size_t componentCount = CalcComponentCount(*record, aComponentID);
			componentCount == 0)
		{
			return nullptr;
		}

		Detail::Archetype* archetype          = record->archetype;
		const Detail::ColumnIndex columnIndex = archetype->columnMap.at(aComponentID);

		return &archetype->components[columnIndex].at<Derived>(record->row);
	}
}
