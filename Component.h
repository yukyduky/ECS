#pragma once
#include "WASD/Component/ComponentInfo.h"

#include <magic_enum/magic_enum.hpp>
#include <tge/WASD/Container/TypelessVector.hpp>

namespace WASD
{
	template<ComponentType T, ComponentType... Dependencies>
	class Component : TypeInfo
	{
	public:
		constexpr Component()                                       = default;
		constexpr Component(Component&& aOther) noexcept            = default;
		constexpr Component(const Component& aOther)                = default;
		constexpr Component& operator=(Component&& aOther) noexcept = default;
		constexpr Component& operator=(const Component& aOther)     = default;
		constexpr ~Component() override                             = default;

		static constexpr ComponentType type                = T;
		static constexpr std::string_view typeString       = magic_enum::enum_name<T>();
		static constexpr ComponentID typeID                = static_cast<ComponentID>(T);
		static constexpr Detail::Dependencies dependencies = Detail::CreateDependencies(static_cast<UniqueID>(Dependencies)...);
	};
}
