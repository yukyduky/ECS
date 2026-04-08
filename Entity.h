#pragma once
#include "WASD/Component/ComponentInfo.h"
#include "WASD/Component/TagComponent.h"
#include "WASD/Component/System/ComponentManager.h"

namespace WASD
{
	// Interface to manipulate the components of an entity
	class Entity
	{
	public :
		Entity(UniqueID aEntity);
		Entity()                                    = default;
		Entity(Entity&& aOther) noexcept            = default;
		Entity(const Entity& aOther)                = default;
		Entity& operator=(Entity&& aOther) noexcept = default;
		Entity& operator=(const Entity& aOther)     = default;
		~Entity()                                   = default;

		static void Init(ComponentManager* aComponentManager);

		// Give it an ID of an existing entity that you want to manipulate
		void Possess(UniqueID aEntityID);
		// Create a new entity
		void Create();
		// Destroy the currently possessed entity
		void Destroy(bool aDestroyChildren = true);

		template<typename T, typename... Args> requires (!std::is_same_v<T, TagComponent>)
		T* AccessOrAddComponent(Args&&... aArgs) const;
		template<typename T>
		void RemoveComponent() const;

		void AddTag(TagID aTag) const;
		void RemoveTag(TagID aTag) const;
		[[nodiscard]] FVector<UniqueID> GetTags() const;

		template<typename T>
		[[nodiscard]] bool HasComponent() const;
		[[nodiscard]] bool HasTag(TagID aTag) const;
		template<typename T>
		[[nodiscard]] const T* GetComponent() const;
		template<typename T>
		[[nodiscard]] T* AccessComponent() const;

		void AddRelationship(UniqueID aRelatedEntity, RelationshipID aRelationship, bool aIsExclusive = {}) const;
		void RemoveRelationship(UniqueID aRelatedEntity, RelationshipID aRelationshipID) const;
		void RemoveExclusiveRelationship(RelationshipID aRelationshipID) const;
		[[nodiscard]] bool HasRelationship(UniqueID aRelatedEntity, RelationshipID aRelationship) const;
		[[nodiscard]] bool HasExclusiveRelationship(RelationshipID aRelationshipID) const;
		[[nodiscard]] UniqueID GetExclusiveRelationship(RelationshipID aRelationshipID) const;
		[[nodiscard]] FVector<UniqueID> GetRelationships(RelationshipID aRelationshipID) const;

		void SetOrAddSwitch(SwitchID aSwitch, CaseID aCase) const;
		void SetOrAddSwitch(SwitchID aSwitch, Entity aCase) const;
		void RemoveSwitch(SwitchID aSwitch) const;
		[[nodiscard]] EntityID GetCase(SwitchID aSwitch) const;
		[[nodiscard]] bool HasSwitch(SwitchID aSwitch) const;

		[[nodiscard]] bool IsAlive() const;

		[[nodiscard]] bool IsValid() const;
		[[nodiscard]] UniqueID GetID() const;
		[[nodiscard]] EntityID GetPartialID() const;

		[[nodiscard]] static EntityID GetOrCreateTag(TagID aTag);
		[[nodiscard]] static EntityID GetOrCreateTag(std::string_view aTag);
		[[nodiscard]] static EntityID GetOrCreateTag(const char* aTag);
		template<typename... Qs, std::size_t N = sizeof...(Qs)> requires ((std::is_convertible_v<Qs, QVariable>) && ...)
		[[nodiscard]] static Entities Query(const QVariable& aSource, const Qs&... aQueries);
		[[nodiscard]] static UniqueID GetFullID(EntityID aPartialID);

		[[nodiscard]] auto operator<=>(const Entity&) const = default;

	private:
		UniqueID myEntityID = INVALID_ID;

		static inline ComponentManager* ourComponentManager = nullptr;
	};

	template<typename T, typename... Args> requires (!std::is_same_v<T, TagComponent>)
	T* Entity::AccessOrAddComponent(Args&&... aArgs) const
	{
		return ourComponentManager->AccessOrAddComponent<T>(myEntityID, std::forward<Args>(aArgs)...);
	}

	template<typename T>
	void Entity::RemoveComponent() const
	{
		ourComponentManager->RemoveComponent<T>(myEntityID);
	}

	template<typename T>
	bool Entity::HasComponent() const
	{
		return ourComponentManager->HasComponent<T>(myEntityID);
	}

	template<typename T>
	const T* Entity::GetComponent() const
	{
		return ourComponentManager->AccessComponent<T>(myEntityID);
	}

	template<typename T>
	T* Entity::AccessComponent() const
	{
		return ourComponentManager->AccessComponent<T>(myEntityID);
	}

	template<typename... Qs, std::size_t N> requires ((std::is_convertible_v<Qs, QVariable>) && ...)
	Entities Entity::Query(const QVariable& aSource, const Qs&... aQueries)
	{
		return ourComponentManager->Query(aSource, aQueries...);
	}
}
