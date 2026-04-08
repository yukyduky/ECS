#include "stdafx.h"
#include "Entity.h"

#include "WASD/Component/TagComponent.h"

WASD::Entity::Entity(const UniqueID aEntity) :
	myEntityID(aEntity)
{
}

void WASD::Entity::Init(ComponentManager* aComponentManager)
{
	ourComponentManager = aComponentManager;
}

void WASD::Entity::Possess(const UniqueID aEntityID)
{
	myEntityID = aEntityID;
}

void WASD::Entity::Create()
{
	myEntityID = ourComponentManager->AddEntity();
}

void WASD::Entity::Destroy(const bool aDestroyChildren)
{
	if (myEntityID == INVALID_ID)
	{
		return;
	}
	ourComponentManager->RemoveEntity(myEntityID, aDestroyChildren);
	myEntityID = INVALID_ID;
}

void WASD::Entity::AddTag(const TagID aTag) const
{
	const EntityID tagID = ourComponentManager->GetOrCreateTag(aTag);
	ourComponentManager->AccessOrAddComponentWithID<TagComponent>(myEntityID, tagID);
}

void WASD::Entity::RemoveTag(const TagID aTag) const
{
	const EntityID tagID = ourComponentManager->GetOrCreateTag(aTag);
	ourComponentManager->RemoveComponent(myEntityID, tagID);
}

FVector<WASD::UniqueID> WASD::Entity::GetTags() const
{
	return ourComponentManager->GetTags(myEntityID);
}

bool WASD::Entity::HasTag(const TagID aTag) const
{
	const EntityID tagID = ourComponentManager->GetOrCreateTag(aTag);
	return ourComponentManager->HasComponent(myEntityID, tagID);
}

void WASD::Entity::AddRelationship(const UniqueID aRelatedEntity, const RelationshipID aRelationship, const bool aIsExclusive) const
{
	const EntityID relationshipID = ourComponentManager->GetOrCreateTag(aRelationship);

	if (aIsExclusive)
	{
		if (const UniqueID searchTerm = static_cast<UniqueID>(relationshipID) << ENTITY_ID_DIGIT_COUNT;
			ourComponentManager->HasComponent(myEntityID, searchTerm))
		{
			ourComponentManager->RemoveComponent(myEntityID, searchTerm);
		}
	}
	const auto relatedEntity  = static_cast<EntityID>(aRelatedEntity);
	const UniqueID combinedID = CombineIDs(relatedEntity, relationshipID);
	ourComponentManager->AccessOrAddComponentWithID<TagComponent>(myEntityID, combinedID);
}

void WASD::Entity::RemoveRelationship(const UniqueID aRelatedEntity, const RelationshipID aRelationshipID) const
{
	const EntityID relationshipID = ourComponentManager->GetOrCreateTag(aRelationshipID);
	const auto relatedEntity      = static_cast<EntityID>(aRelatedEntity);
	const UniqueID combinedID     = CombineIDs(relatedEntity, relationshipID);
	ourComponentManager->RemoveComponent(myEntityID, combinedID);
}

void WASD::Entity::RemoveExclusiveRelationship(const RelationshipID aRelationshipID) const
{
	const FVector relationships = ourComponentManager->GetRelationships(myEntityID, aRelationshipID, Detail::TypeFlag::Exclusive);
#ifndef _RETAIL
	if (relationships.size() > 1)
	{
		LOG_WARNING("Entity has more than one relationship that is meant to be exclusive");
	}
#endif
	ourComponentManager->RemoveComponent(myEntityID, relationships.front());
}

bool WASD::Entity::HasRelationship(const UniqueID aRelatedEntity, const RelationshipID aRelationship) const
{
	const EntityID relationshipID = ourComponentManager->GetOrCreateTag(aRelationship);
	const auto relatedEntity      = static_cast<EntityID>(aRelatedEntity);
	const UniqueID combinedID     = CombineIDs(relatedEntity, relationshipID);
	return ourComponentManager->HasComponent(myEntityID, combinedID);
}

bool WASD::Entity::HasExclusiveRelationship(const RelationshipID aRelationshipID) const
{
	const auto relationships = ourComponentManager->GetRelationships(myEntityID, aRelationshipID, Detail::TypeFlag::Exclusive);
	return !relationships.empty();
}

WASD::UniqueID WASD::Entity::GetExclusiveRelationship(const RelationshipID aRelationshipID) const
{
	const auto relationships = ourComponentManager->GetRelationships(myEntityID, aRelationshipID, Detail::TypeFlag::Exclusive);
	return relationships.empty() ? INVALID_ID : relationships.front();
}

FVector<WASD::UniqueID> WASD::Entity::GetRelationships(const RelationshipID aRelationshipID) const
{
	return ourComponentManager->GetRelationships(myEntityID, aRelationshipID);
}

void WASD::Entity::SetOrAddSwitch(const SwitchID aSwitch, const CaseID aCase) const
{
	const EntityID switchID   = ourComponentManager->GetOrCreateTag(aSwitch);
	const EntityID caseID     = ourComponentManager->GetOrCreateTag(aCase);
	const UniqueID combinedID = CombineIDs(caseID, switchID, Detail::TypeFlag::Switch);
	ourComponentManager->AccessOrAddComponentWithID<TagComponent>(myEntityID, combinedID);
}

void WASD::Entity::SetOrAddSwitch(const SwitchID aSwitch, const Entity aCase) const
{
	const EntityID switchID   = ourComponentManager->GetOrCreateTag(aSwitch);
	const UniqueID combinedID = CombineIDs(aCase.GetPartialID(), switchID, Detail::TypeFlag::Switch);
	ourComponentManager->AccessOrAddComponentWithID<TagComponent>(myEntityID, combinedID);
}

void WASD::Entity::RemoveSwitch(const SwitchID aSwitch) const
{
	const EntityID switchID   = ourComponentManager->GetOrCreateTag(aSwitch);
	const UniqueID combinedID = CombineIDs(0, switchID, Detail::TypeFlag::Switch);
	ourComponentManager->RemoveComponent(myEntityID, combinedID);
}

WASD::EntityID WASD::Entity::GetCase(const SwitchID aSwitch) const
{
	const EntityID switchID = ourComponentManager->GetOrCreateTag(aSwitch);
	return ourComponentManager->GetCase(myEntityID, switchID);
}

bool WASD::Entity::HasSwitch(const SwitchID aSwitch) const
{
	const EntityID switchID   = ourComponentManager->GetOrCreateTag(aSwitch);
	const UniqueID combinedID = CombineIDs(0, switchID, Detail::TypeFlag::Switch);
	return ourComponentManager->HasComponent(myEntityID, combinedID);
}

bool WASD::Entity::IsAlive() const
{
	return ourComponentManager->IsAlive(myEntityID);
}

bool WASD::Entity::IsValid() const
{
	return myEntityID != INVALID_ID;
}

WASD::UniqueID WASD::Entity::GetID() const
{
	return myEntityID;
}

WASD::EntityID WASD::Entity::GetPartialID() const
{
	return GetComponentID(myEntityID);
}

WASD::EntityID WASD::Entity::GetOrCreateTag(const TagID aTag)
{
	return ourComponentManager->GetOrCreateTag(aTag);
}

WASD::EntityID WASD::Entity::GetOrCreateTag(const std::string_view aTag)
{
	return ourComponentManager->GetOrCreateTag(aTag);
}

WASD::EntityID WASD::Entity::GetOrCreateTag(const char* aTag)
{
	return ourComponentManager->GetOrCreateTag(aTag);
}

WASD::UniqueID WASD::Entity::GetFullID(const EntityID aPartialID)
{
	return ourComponentManager->GetFullID(aPartialID);
}
