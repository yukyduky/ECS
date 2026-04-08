#include "stdafx.h"
#include "TransformComponent.h"

#include "WASD/Component/System/ComponentManager.h"

WASD::TransformComponent::TransformComponent(const Tga::TRSf& aTRS, const Tga::Vector3f& aPivot) :
	myData(std::forward_as_tuple(aTRS, aPivot))
{
}

WASD::TransformComponent::TransformComponent(const Tga::Vector3f& aPosition, const Tga::Quatf& aRotation, const Tga::Vector3f& aScale,
	const Tga::Vector3f& aPivot) :
	myData(std::forward_as_tuple(Tga::TRSf{aPosition, aRotation, aScale}, aPivot))
{
}

WASD::TransformComponent::TransformComponent(const Tga::Vector3f& aPosition, const Tga::Vector3f& aRotation, const Tga::Vector3f& aScale,
	const Tga::Vector3f& aPivot) :
	myData(std::forward_as_tuple(Tga::TRSf{aPosition, Tga::Quatf{aRotation}, aScale}, aPivot))
{
}

void WASD::TransformComponent::SetTRS(const Tga::TRSf& aTRS)
{
	myData.Write().trs = aTRS;
}

void WASD::TransformComponent::SetPivot(const Tga::Vector3f& aPivot)
{
	myData.Write().pivot = aPivot;
}

void WASD::TransformComponent::SetPosition(const Tga::Vector3f& aPosition)
{
	myData.Write().trs.SetPosition(aPosition);
}

void WASD::TransformComponent::SetRotation(const Tga::Quatf& aRotation)
{
	myData.Write().trs.SetRotation(aRotation);
}

void WASD::TransformComponent::SetRotation(const Tga::Vector3f& aPitchYawRoll)
{
	myData.Write().trs.SetRotation(Tga::Quatf(aPitchYawRoll));
}

void WASD::TransformComponent::SetScale(const Tga::Vector3f& aScale)
{
	myData.Write().trs.SetScale(aScale);
}

const Tga::TRSf& WASD::TransformComponent::GetTRS() const
{
	return myData.Read().trs;
}

Tga::TRSf& WASD::TransformComponent::AccessTRS()
{
	return myData.Write().trs;
}

const Tga::Vector3f& WASD::TransformComponent::GetPivot() const
{
	return myData.Read().pivot;
}

const Tga::Vector3f& WASD::TransformComponent::GetPosition() const
{
	return myData.Read().trs.GetPosition();
}

const Tga::Quatf& WASD::TransformComponent::GetRotation() const
{
	return myData.Read().trs.GetRotation();
}

const Tga::Vector3f& WASD::TransformComponent::GetScale() const
{
	return myData.Read().trs.GetScale();
}

bool WASD::TransformComponent::IsStatic() const
{
	return myIsStatic;
}

void WASD::TransformComponent::SetIsStatic(const bool aIsStatic)
{
	myIsStatic = aIsStatic;
}

void WASD::TransformComponent::MakeDirty() const
{
	myData.MakeDirty();
}

bool WASD::TransformComponent::IsDirty() const
{
	return myData.IsDirty();
}

void WASD::TransformComponent::Clean() const
{
	myData.Clean();
}

WASD::Detail::ComponentTypeData WASD::TransformComponent::GetComponentTypeData()
{
	return {
		.id = typeID,
		.size = static_cast<unsigned>(sizeof(TransformComponent)),
		.dependencies = dependencies,
		.dependants = ComponentManager::GetDependantComponents(typeID),
		.constructFunc = [](Detail::Column& aComponentColumn)
		{
			aComponentColumn.emplace_back<TransformComponent>();
		},
		.transferFunc = [](Detail::Column& aToColumn, Detail::Column& aFromColumn, const std::size_t aFromSlot)
		{
			auto& component = aFromColumn.at<TransformComponent>(aFromSlot);
			aToColumn.emplace_back<TransformComponent>(std::move(component));
			aFromColumn.move_pop<TransformComponent>(aFromSlot);
		},
		.movePopFunc = [](Detail::Column& aComponentColumn, const std::size_t aIndex)
		{
			aComponentColumn.move_pop<TransformComponent>(aIndex);
		}
	};
}

void WASD::TransformComponent::Destruct()
{
	this->~TransformComponent();
}
