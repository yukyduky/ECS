#pragma once
#include "Component.h"

#include <tge/math/ScaleRotationTranslation.h>
#include <tge/WASD/Util/DataWatcher.h>

namespace WASD
{
	class TransformComponent final : public Component<ComponentType::Transform>
	{
	public:
		TransformComponent(const Tga::TRSf& aTRS, const Tga::Vector3f& aPivot = {});
		TransformComponent(const Tga::Vector3f& aPosition, const Tga::Quatf& aRotation, const Tga::Vector3f& aScale,
			const Tga::Vector3f& aPivot = {});
		TransformComponent(const Tga::Vector3f& aPosition, const Tga::Vector3f& aRotation, const Tga::Vector3f& aScale,
			const Tga::Vector3f& aPivot = {});
		TransformComponent()                                                             = default;
		TransformComponent(TransformComponent&& aTransformComponent) noexcept            = default;
		TransformComponent(const TransformComponent& aTransformComponent)                = default;
		TransformComponent& operator=(TransformComponent&& aTransformComponent) noexcept = default;
		TransformComponent& operator=(const TransformComponent& aTransformComponent)     = default;
		~TransformComponent() override                                                   = default;

		void SetTRS(const Tga::TRSf& aTRS);
		void SetPivot(const Tga::Vector3f& aPivot);
		void SetPosition(const Tga::Vector3f& aPosition);
		void SetRotation(const Tga::Quatf& aRotation);
		void SetRotation(const Tga::Vector3f& aPitchYawRoll);
		void SetScale(const Tga::Vector3f& aScale);

		const Tga::TRSf& GetTRS() const;
		Tga::TRSf& AccessTRS();
		const Tga::Vector3f& GetPivot() const;
		const Tga::Vector3f& GetPosition() const;
		const Tga::Quatf& GetRotation() const;
		const Tga::Vector3f& GetScale() const;

		bool IsStatic() const;
		void SetIsStatic(bool aIsStatic);

		void MakeDirty() const;
		bool IsDirty() const;
		void Clean() const;

		[[nodiscard]] static Detail::ComponentTypeData GetComponentTypeData();

		void Destruct() override;

	private:
		struct Data
		{
			Tga::TRSf trs;
			Tga::Vector3f pivot;
		};

		DataWatcher<Data> myData;

		bool myIsStatic = false;
	};
}
