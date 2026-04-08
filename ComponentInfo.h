#pragma once
#include <functional>
#include <limits>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>
#include <magic_enum/magic_enum.hpp>
#include <sul/dynamic_bitset.hpp>
#include <tge/stringRegistry/StringRegistry.h>
#include <tge/WASD/StandardTypes.h>
#include <tge/WASD/Container/TypelessVector.hpp>
#include <tge/WASD/Memory/frame_containers.h>

namespace ME = magic_enum;
using namespace ME::bitwise_operators;

namespace WASD
{
	using UniqueID       = uint64_t;
	using EntityID       = uint32_t;
	using ArchetypeID    = uint64_t;
	using ComponentID    = EntityID;
	using RelationshipID = Tga::StringId;
	using TagID          = Tga::StringId;
	using SwitchID       = Tga::StringId;
	using CaseID         = Tga::StringId;

	enum class ComponentType : uint8_t
	{
		Wildcard,
		//Any,
		Transform,
		Model,
		Animation,
		Action,
		Collision,
		Behaviour,
		SoftAreaLight,
		PointLight,
		SpotLight,
		DirLight,
		Decal,
		UI,
		Flipbook,
		Lifetime,
		Text,
		Video,
		Script,
		BehaviourTree,
		Idle,
		ShootProjectile,
		MoveOnNavmesh,
		AIDebugAlert,
		RigidBody,
		PlayerData,
		Health,
		HealthChangeEvent,
		Prefab,
		Teleport,
		Timer,
		MoveInDir,
		ProjectileCollision,
		WeaponContainer,
		Pickup,
		Spawner,
		Widget,
		Tag // Has to be last
	};

	enum class TraitID : uint8_t
	{
		Wildcard,
		//Any,
		HasInstanceOf,
		ChildOf,
		IsA,
		LocatedIn,
		Faction,
		AlliedWith,
		AtWar,
		TradesWith,
		// Add any trait you might find useful, imagination is the only limit
	};

	namespace Detail
	{
		enum class TypeFlag : uint8_t
		{
			Enabled   = 1u << 0u,
			Exclusive = 1u << 1u,
			Sparse    = 1u << 2u,
			Switch    = 1u << 3u,
		};

		constexpr uint32_t TYPE_FLAG_COUNT = 5;
	}

	constexpr uint8_t MAX_COMPONENT_DEPENDENCIES_COUNT = 4;

	constexpr auto UNIQUE_ID_DIGIT_COUNT = static_cast<uint32_t>(std::numeric_limits<UniqueID>::digits);
	constexpr auto ENTITY_ID_DIGIT_COUNT = static_cast<uint32_t>(std::numeric_limits<EntityID>::digits);

	[[nodiscard]] constexpr EntityID GetComponentID(const UniqueID aID)
	{
		return static_cast<EntityID>(aID);
	}

	[[nodiscard]] constexpr EntityID GetQualifierID(const UniqueID aID)
	{
		return static_cast<EntityID>(aID >> ENTITY_ID_DIGIT_COUNT) & ~static_cast<EntityID>(0) >> Detail::TYPE_FLAG_COUNT;
	}

	[[nodiscard]] constexpr UniqueID CombineIDs(const EntityID aComponentID, const EntityID aQualifier, const Detail::TypeFlag aFlags = {})
	{
		UniqueID combinedID = ME::enum_integer(aFlags);
		combinedID          <<= ENTITY_ID_DIGIT_COUNT - Detail::TYPE_FLAG_COUNT;
		combinedID          |= aQualifier;
		combinedID          <<= ENTITY_ID_DIGIT_COUNT;
		combinedID          |= aComponentID;
		return combinedID;
	}

	[[nodiscard]] constexpr UniqueID CreateQualifier(const EntityID aQualifier)
	{
		UniqueID qualifierID = aQualifier;
		qualifierID          <<= ENTITY_ID_DIGIT_COUNT;
		return qualifierID;
	}

	using ArchetypeEntities = std::vector<UniqueID>;
	using Entities          = FVector<UniqueID>;

	namespace Detail
	{
		using RowID        = uint64_t;
		using GenerationID = uint16_t;
		using ColumnIndex  = std::size_t;
		using IndexSize    = uint32_t;

		using Column = TypelessVector;
		using Type   = std::vector<UniqueID>;

		struct Archetype;

		constexpr UniqueID COMPONENT_MASK = GetComponentID(~static_cast<UniqueID>(0));
		constexpr UniqueID QUALIFIER_MASK = static_cast<UniqueID>(GetQualifierID(~static_cast<UniqueID>(0))) << ENTITY_ID_DIGIT_COUNT;
		constexpr UniqueID TYPE_FLAG_MASK = ~static_cast<UniqueID>(0) << (UNIQUE_ID_DIGIT_COUNT - TYPE_FLAG_COUNT);

		[[nodiscard]] constexpr UniqueID ClearComponentFlags(const UniqueID aID)
		{
			return aID & (QUALIFIER_MASK | COMPONENT_MASK);
		}

		[[nodiscard]] constexpr TypeFlag GetComponentFlags(const UniqueID aID)
		{
			return static_cast<TypeFlag>(aID >> (UNIQUE_ID_DIGIT_COUNT - TYPE_FLAG_COUNT));
		}

		[[nodiscard]] constexpr IndexSize GetCaseIndex(const UniqueID aID)
		{
			return static_cast<IndexSize>(aID) - static_cast<IndexSize>(ME::enum_count<ComponentType>());
		}

		[[nodiscard]] constexpr bool IsFlagSet(const UniqueID aID, const TypeFlag aFlag)
		{
			const auto flags = GetComponentFlags(aID);
			return (flags & aFlag) == aFlag;
		}

		[[nodiscard]] constexpr UniqueID InsertSwitchID(const UniqueID aID, const UniqueID aSwitchID)
		{
			return (aID & ~COMPONENT_MASK) | aSwitchID;
		}

		template<typename Derived> concept isComponent = std::is_base_of_v<TypeInfo, Derived>;

		using ConstructComponentFunc = std::function<void(Column&)>;
		using TransferComponentFunc  = std::function<void(Column&, Column&, std::size_t)>;
		using MovePopComponentFunc   = std::function<void(Column&, std::size_t)>;

		constexpr std::size_t DEPENDENCIES_MAX_COUNT = ME::enum_count<ComponentType>();

		struct Dependencies
		{
			std::array<UniqueID, DEPENDENCIES_MAX_COUNT> types{};
			unsigned count = 0;
		};

		struct ComponentTypeData
		{
			UniqueID id   = 0;
			unsigned size = 0;
			Dependencies dependencies;
			Dependencies dependants;
			ConstructComponentFunc constructFunc;
			TransferComponentFunc transferFunc;
			MovePopComponentFunc movePopFunc;
		};

		struct CaseNode
		{
			EntityID value      = std::numeric_limits<EntityID>::max();
			IndexSize nextIndex = 0;
			IndexSize prevIndex = 0;
		};

		struct SwitchList
		{
			std::unordered_map<EntityID, IndexSize> headCaseIndex;
			std::vector<CaseNode> caseList; // TODO: Change to sparse vector
		};

		struct ArchetypeEdge
		{
			Archetype* add    = nullptr;
			Archetype* remove = nullptr;
		};

		struct Archetype
		{
			std::unordered_map<UniqueID, ArchetypeEdge> edges;
			std::unordered_map<UniqueID, ColumnIndex> columnMap;
			std::vector<Column> components;
			Type type;
			ArchetypeEntities entities;
			ArchetypeID id = std::numeric_limits<ArchetypeID>::max();
		};

		struct ArchetypeRecord
		{
			Archetype* archetype = nullptr;
			std::size_t count    = 0;
		};

		struct Record
		{
			Archetype* archetype = nullptr;
			RowID row            = 0;
			EntityID entityIndex = 0;
		};

		struct EntityIndex
		{
			EntityID alive = 0;
			ArchetypeEntities entities;
			std::unordered_map<EntityID, Record> records;
		};

		using ArchetypeMap = std::unordered_map<ArchetypeID, ArchetypeRecord>;

		struct TypeHasher
		{
			UniqueID operator()(const Type& aType) const
			{
				UniqueID hash = aType.size();
				for (const UniqueID& i : aType)
				{
					hash += static_cast<UniqueID>(i);
				}
				return hash;
			}
		};

		template<typename T> concept isDependency = std::is_same_v<std::remove_cvref_t<T>, UniqueID>;

		consteval Dependencies CreateDependencies(const auto&... aDependencies) requires ((isDependency<decltype(aDependencies)> && ...))
		{
			return {.types = {aDependencies...}, .count = static_cast<unsigned>(sizeof...(aDependencies))};
		}

		consteval Dependencies GetDependencyCount(const std::array<UniqueID, DEPENDENCIES_MAX_COUNT>& aDependencies)
		{
			Dependencies dependencies = {.types = aDependencies, .count = 0};
			const std::size_t count   = std::ranges::count_if(aDependencies, [](const UniqueID aDependencyID)
			{
				return aDependencyID != INVALID_ID;
			});
			dependencies.count = static_cast<unsigned>(count);
			return dependencies;
		}

		constexpr ArchetypeID INVALID_ARCHETYPE_ID = std::numeric_limits<ArchetypeID>::max();
		// ZSC: Zero Sized Component: A component that does not have a physical component
		// it is only an ID that has contains data within itself
		constexpr ColumnIndex ZSC_COMPONENT_COLUMN = std::numeric_limits<ColumnIndex>::max();
	}

	template<typename T> concept isID = std::is_unsigned_v<std::underlying_type_t<T>>;

	template<typename T> concept isComponentID = std::is_same_v<std::remove_cvref_t<T>, UniqueID> || std::is_same_v<
		                                             std::remove_cvref_t<T>, EntityID> || std::is_same_v<
		                                             std::remove_cvref_t<T>, ComponentType>;

	class QTerm
	{
	public:
		constexpr QTerm() = default;

		constexpr QTerm(const auto aID) requires (isComponentID<decltype(aID)>) :
			value(static_cast<EntityID>(aID))
		{
		}

		constexpr QTerm(const auto aQualifier, const auto aID) requires (
			isComponentID<decltype(aID)> && !isComponentID<decltype(aQualifier)>) :
			value(CombineIDs(static_cast<EntityID>(aID), static_cast<EntityID>(aQualifier)))
		{
		}

		constexpr QTerm(const auto aQualifier) requires (!isComponentID<decltype(aQualifier)>) :
			value(CreateQualifier(static_cast<UniqueID>(aQualifier)))
		{
		}

		constexpr QTerm operator!() const
		{
			QTerm arg(*this);
			arg.isWith = !arg.isWith;
			return arg;
		}

		constexpr operator bool() const
		{
			return isWith;
		}

		constexpr operator UniqueID() const
		{
			return value;
		}

		UniqueID value = 0;
		bool isWith    = true;
	};

	class QVariable
	{
	public:
		constexpr QVariable() = default;

		constexpr QVariable(const auto aID) requires (isComponentID<decltype(aID)>) :
			term(aID)
		{
		}

		constexpr QVariable(const auto aQualifier) requires (!isComponentID<decltype(aQualifier)>) :
			term(aQualifier)
		{
		}

		constexpr QVariable(const auto aQualifier, const auto aID) requires (!isComponentID<decltype(aQualifier)>) :
			term(aQualifier, aID)
		{
		}

		constexpr QVariable(const UniqueID aSource, const auto aQualifier, const auto aID) requires (!isComponentID<decltype(aQualifier)>) :
			term(aQualifier, aID),
			source(aSource)
		{
		}

		constexpr QVariable(const UniqueID aSource, const auto aQualifier) requires (!isComponentID<decltype(aQualifier)>) :
			term(aQualifier),
			source(aSource)
		{
		}

		constexpr QVariable(const UniqueID aSource, const QTerm aTerm) :
			term(aTerm),
			source(aSource)
		{
		}

		constexpr QVariable operator!() const
		{
			QVariable variable(*this);
			variable.term.isWith = !variable.term.isWith;
			return variable;
		}

		constexpr operator bool() const
		{
			return term.isWith;
		}

		QTerm term;
		UniqueID source = 0;
	};
}
