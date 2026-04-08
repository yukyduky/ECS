#pragma once
#include "tge/WASD/Error/Assert_WASD.h"

#include <vector>

// ReSharper disable CppInconsistentNaming
// ReSharper disable CppClangTidyBugproneCastingThroughVoid

class TypelessVector;
class TypeInfo;

constexpr unsigned TYPELESS_VECTOR_ALIGNMENT_SIZE = 16;

class TypeInfo
{
public:
	constexpr TypeInfo()                                         = default;
	constexpr TypeInfo(TypeInfo&& aTypeInfo) noexcept            = default;
	constexpr TypeInfo(const TypeInfo& aTypeInfo)                = default;
	constexpr TypeInfo& operator=(TypeInfo&& aTypeInfo) noexcept = default;
	constexpr TypeInfo& operator=(const TypeInfo& aTypeInfo)     = default;
	virtual constexpr ~TypeInfo()                                = default;

	virtual constexpr void Destruct() = 0;
};

template<typename Derived> concept TypeInfoDerived = std::is_base_of_v<TypeInfo, Derived>;

class TypelessVector final
{
public:
	enum class PaddingType : uint8_t
	{
		Vector,
		VectorOffset,
		PerType,
		TotalType,
		Total
	};

	TypelessVector(unsigned aTypeSize);
	TypelessVector(TypelessVector&& aTypelessVector) noexcept;
	template<TypeInfoDerived Type>
	TypelessVector(const TypelessVector& aTypelessVector);
	TypelessVector& operator=(TypelessVector&& aTypelessVector) noexcept;
	template<TypeInfoDerived Type>
	TypelessVector& operator=(const TypelessVector& aTypelessVector);
	~TypelessVector();

	template<TypeInfoDerived Type>
	[[nodiscard]] Type& at(std::size_t aIndex);
	template<TypeInfoDerived Type>
	[[nodiscard]] const Type& at(std::size_t aIndex) const;
	template<TypeInfoDerived Type>
	[[nodiscard]] Type& front();
	template<TypeInfoDerived Type>
	[[nodiscard]] const Type& front() const;
	template<TypeInfoDerived Type>
	[[nodiscard]] Type& back();
	template<TypeInfoDerived Type>
	[[nodiscard]] const Type& back() const;
	template<TypeInfoDerived Type>
	[[nodiscard]] Type* data();
	template<TypeInfoDerived Type>
	[[nodiscard]] const Type* data() const;
	template<TypeInfoDerived Type>
	[[nodiscard]] Type* begin();
	template<TypeInfoDerived Type>
	[[nodiscard]] const Type* begin() const;
	template<TypeInfoDerived Type>
	[[nodiscard]] const Type* cbegin() const;
	template<TypeInfoDerived Type>
	[[nodiscard]] Type* end();
	template<TypeInfoDerived Type>
	[[nodiscard]] const Type* end() const;
	template<TypeInfoDerived Type>
	[[nodiscard]] const Type* cend() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t size() const;
	[[nodiscard]] std::size_t padding(PaddingType aPaddingType) const;
	template<TypeInfoDerived Type, class... Args>
	void reserve(std::size_t aNewCapacity);
	[[nodiscard]] std::size_t capacity() const;
	void clear();
	template<TypeInfoDerived Type>
	void move_slot(const TypelessVector& other, std::size_t aIndex);
	template<TypeInfoDerived Type, class... Args>
	Type& emplace_back(Args&&... aArgs);
	void pop_back();
	template<TypeInfoDerived Type>
	void move_pop(std::size_t aIndex);
	template<TypeInfoDerived Type, class... Args>
	void resize(std::size_t aNewSize, Args&&... aArgs);

private:
	[[nodiscard]] std::byte* GetPtrFromIndex(std::size_t aIndex) const;
	void DestructTypeAtIndex(std::size_t aIndex) const;
	static unsigned CalcSlotSize(unsigned aTypeSize);

	std::vector<std::byte> myBuffer;
	std::size_t mySize     = 0;
	std::size_t myCapacity = 0;
	unsigned myTypeSize    = 0;
	unsigned mySlotSize    = 0;
	std::byte* myBeginPtr  = nullptr;
};

inline TypelessVector::TypelessVector(const unsigned aTypeSize) :
	myTypeSize(aTypeSize),
	mySlotSize(CalcSlotSize(aTypeSize))
{
}

inline TypelessVector::TypelessVector(TypelessVector&& aTypelessVector) noexcept :
	myBuffer(std::move(aTypelessVector.myBuffer)),
	mySize(aTypelessVector.mySize),
	myCapacity(aTypelessVector.myCapacity),
	myTypeSize(aTypelessVector.myTypeSize),
	mySlotSize(CalcSlotSize(aTypelessVector.myTypeSize)),
	myBeginPtr(aTypelessVector.myBeginPtr)
{
	aTypelessVector.myBeginPtr = nullptr;
	aTypelessVector.myCapacity = 0;
	aTypelessVector.mySize     = 0;
}

template<TypeInfoDerived Type>
TypelessVector::TypelessVector(const TypelessVector& aTypelessVector) :
	myTypeSize(aTypelessVector.myTypeSize),
	mySlotSize(CalcSlotSize(aTypelessVector.myTypeSize))

{
	reserve<Type>(aTypelessVector.capacity());
	for (std::size_t i = 0; i < aTypelessVector.size(); ++i)
	{
		emplace_back<Type>(aTypelessVector.at<Type>(i));
	}
}

inline TypelessVector& TypelessVector::operator=(TypelessVector&& aTypelessVector) noexcept
{
	if (this == &aTypelessVector)
	{
		return *this;
	}
	ASSERT_WASD(myTypeSize == aTypelessVector.myTypeSize, "AlignedVector does not have the same type size; It has been overridden",
	            AssertFlags_SystemMalfunction);

	myBuffer                   = std::move(aTypelessVector.myBuffer);
	mySize                     = aTypelessVector.mySize;
	myCapacity                 = aTypelessVector.myCapacity;
	myBeginPtr                 = aTypelessVector.myBeginPtr;
	aTypelessVector.myBeginPtr = nullptr;
	aTypelessVector.myCapacity = 0;
	aTypelessVector.mySize     = 0;
	return *this;
}

template<TypeInfoDerived Type>
TypelessVector& TypelessVector::operator=(const TypelessVector& aTypelessVector)
{
	if (this == &aTypelessVector)
	{
		return *this;
	}
	ASSERT_WASD(myTypeSize == aTypelessVector.myTypeSize, "AlignedVector does not have the same type size; It has been overridden",
	            AssertFlags_SystemMalfunction);

	clear();
	reserve<Type>(aTypelessVector.capacity());
	for (std::size_t i = 0; i < aTypelessVector.size(); ++i)
	{
		emplace_back<Type>(aTypelessVector.at<Type>(i));
	}

	return *this;
}

inline TypelessVector::~TypelessVector()
{
	clear();
}

template<TypeInfoDerived Type>
Type& TypelessVector::at(const std::size_t aIndex)
{
	ASSERT_WASD(aIndex < mySize, "Index is out of bounds.", AssertFlags_SystemMalfunction);
	return *static_cast<Type*>(static_cast<void*>(GetPtrFromIndex(aIndex)));
}

template<TypeInfoDerived Type>
const Type& TypelessVector::at(const std::size_t aIndex) const
{
	ASSERT_WASD(aIndex < mySize, "Index is out of bounds.", AssertFlags_SystemMalfunction);
	return *static_cast<const Type*>(static_cast<const void*>(GetPtrFromIndex(aIndex)));
}

template<TypeInfoDerived Type>
Type& TypelessVector::front()
{
	ASSERT_WASD(!empty(), "Tried to access first element when vector was empty", AssertFlags_SystemMalfunction);
	return *myBeginPtr;
}

template<TypeInfoDerived Type>
const Type& TypelessVector::front() const
{
	ASSERT_WASD(!empty(), "Tried to access first element when vector was empty", AssertFlags_SystemMalfunction);
	return *myBeginPtr;
}

template<TypeInfoDerived Type>
Type& TypelessVector::back()
{
	ASSERT_WASD(!empty(), "Tried to access last element when vector was empty", AssertFlags_SystemMalfunction);
	return at<Type>(mySize - 1);
}

template<TypeInfoDerived Type>
const Type& TypelessVector::back() const
{
	ASSERT_WASD(!empty(), "Tried to access last element when vector was empty", AssertFlags_SystemMalfunction);
	return at<Type>(mySize - 1);
}

template<TypeInfoDerived Type>
Type* TypelessVector::data()
{
	return begin<Type>();
}

template<TypeInfoDerived Type>
const Type* TypelessVector::data() const
{
	return cbegin<Type>();
}

template<TypeInfoDerived Type>
Type* TypelessVector::begin()
{
	return static_cast<Type*>(static_cast<void*>(myBeginPtr));
}

template<TypeInfoDerived Type>
const Type* TypelessVector::begin() const
{
	return static_cast<Type*>(static_cast<void*>(myBeginPtr));
}

template<TypeInfoDerived Type>
const Type* TypelessVector::cbegin() const
{
	return static_cast<const Type*>(static_cast<const void*>(myBeginPtr));
}

template<TypeInfoDerived Type>
Type* TypelessVector::end()
{
	return static_cast<Type*>(static_cast<void*>(GetPtrFromIndex(mySize)));
}

template<TypeInfoDerived Type>
const Type* TypelessVector::end() const
{
	return static_cast<const Type*>(static_cast<const void*>(GetPtrFromIndex(mySize)));
}

template<TypeInfoDerived Type>
const Type* TypelessVector::cend() const
{
	return static_cast<const Type*>(static_cast<const void*>(GetPtrFromIndex(mySize)));
}

inline bool TypelessVector::empty() const
{
	return mySize == 0;
}

inline std::size_t TypelessVector::size() const
{
	return mySize;
}

inline std::size_t TypelessVector::padding(const PaddingType aPaddingType) const
{
	switch (aPaddingType)
	{
		case PaddingType::Vector:
		{
			return (myBuffer.capacity() - (myCapacity * mySlotSize));
		}
		case PaddingType::VectorOffset:
		{
			return (myBeginPtr - myBuffer.data());
		}
		case PaddingType::PerType:
		{
			return mySlotSize - myTypeSize;
		}
		case PaddingType::TotalType:
		{
			return static_cast<std::size_t>(mySlotSize - myTypeSize) * myCapacity;
		}
		case PaddingType::Total:
		{
			return padding(PaddingType::Vector) + padding(PaddingType::TotalType);
		}
	}
	return static_cast<std::size_t>(-1);
}

template<TypeInfoDerived Type, class... Args>
void TypelessVector::reserve(const std::size_t aNewCapacity)
{
	if (myCapacity < aNewCapacity)
	{
		const auto slotSize          = static_cast<std::size_t>(mySlotSize);
		constexpr auto alignmentSize = static_cast<std::size_t>(TYPELESS_VECTOR_ALIGNMENT_SIZE);
		// +1 alignment size to make sure that the capacity requested fits when aligned
		const std::size_t newByteSize = aNewCapacity * slotSize + alignmentSize;
		std::vector<std::byte> newBuffer;
		newBuffer.resize(newByteSize);

		void* beginPtr    = newBuffer.data();
		std::size_t space = newBuffer.capacity();

		// Tries to align beginPtr to specified alignment for myCapacity * myBaseTypeSize amount of memory needed, where space is the total size of memory given
		const void* aligned = std::align(alignmentSize, space - alignmentSize, beginPtr, space);
		aligned;
		ASSERT_WASD(aligned != nullptr, "Failed to align memory", AssertFlags_SystemMalfunction);

		// Amount of Types that can fit into the new space
		myCapacity = space / slotSize;

		auto* it = static_cast<std::byte*>(beginPtr);
		for (std::size_t i = 0; i < mySize; ++i)
		{
			auto* typeIt = static_cast<Type*>(static_cast<void*>(it));
			Type& object = at<Type>(i);
			new(typeIt) Type(std::move(object));
			it += slotSize;
		}

		// Pointer is now aligned
		myBeginPtr = static_cast<std::byte*>(beginPtr);
		myBuffer   = std::move(newBuffer);

		// The difference between myBuffer.capacity() and space is the padding from aligning (space has been given the size of memory used by std::align)
		/*LOG_DEBUG("TypelessVector alignment padding:\nVector: {}\nVector offset: {}\nPer type: {}\nType total: {}\nEverything: {}",
		          padding(PaddingType::Vector), padding(PaddingType::VectorOffset), padding(PaddingType::PerType),
		          padding(PaddingType::TotalType), padding(PaddingType::Total));*/
	}
}

inline std::size_t TypelessVector::capacity() const
{
	return myCapacity;
}

inline void TypelessVector::clear()
{
	for (std::size_t i = 0; i < mySize; ++i)
	{
		DestructTypeAtIndex(i);
	}
	mySize = 0;
}

template<TypeInfoDerived Type>
void TypelessVector::move_slot(const TypelessVector& other, const std::size_t aIndex)
{
	Type& data = other.at<Type>(aIndex);
	emplace_back<Type>(std::move(data));
}

template<TypeInfoDerived Type, class... Args>
Type& TypelessVector::emplace_back(Args&&... aArgs)
{
	if (mySize == myCapacity)
	{
		if (mySize == 0)
		{
			reserve<Type>(1);
		}
		else
		{
			reserve<Type>(mySize * 2ull);
		}
	}
	new(end<Type>()) Type(std::forward<Args>(aArgs)...);

	const std::size_t index = mySize++;

	return *static_cast<Type*>(static_cast<void*>(GetPtrFromIndex(index)));
}

inline void TypelessVector::pop_back()
{
	ASSERT_WASD(!empty(), "Tried to pop element when vector was empty", AssertFlags_SystemMalfunction);
	--mySize;
	DestructTypeAtIndex(mySize);
}

template<TypeInfoDerived Type>
void TypelessVector::move_pop(const std::size_t aIndex)
{
	ASSERT_WASD(aIndex < mySize, "Index is out of bounds.", AssertFlags_SystemMalfunction);
	at<Type>(aIndex) = std::move(back<Type>());
	pop_back();
}

template<TypeInfoDerived Type, class... Args>
void TypelessVector::resize(const std::size_t aNewSize, Args&&... aArgs)
{
	if (mySize < aNewSize)
	{
		if (myCapacity < aNewSize)
		{
			reserve<Type>(aNewSize);
		}

		for (std::size_t i = mySize; i < aNewSize; ++i)
		{
			new(GetPtrFromIndex(i)) Type(std::forward<Args>(aArgs)...);
		}
		mySize = aNewSize;
	}
	else if (mySize > aNewSize)
	{
		for (std::size_t i = aNewSize; i < mySize; ++i)
		{
			at<Type>(i).~Type();
		}
		mySize = aNewSize;
	}
}

inline std::byte* TypelessVector::GetPtrFromIndex(const std::size_t aIndex) const
{
	return myBeginPtr + aIndex * mySlotSize;
}

inline void TypelessVector::DestructTypeAtIndex(const std::size_t aIndex) const
{
	static_cast<TypeInfo*>(static_cast<void*>(GetPtrFromIndex(aIndex)))->Destruct();
}

inline unsigned TypelessVector::CalcSlotSize(const unsigned aTypeSize)
{
	// Round up to closest alignment size
	constexpr auto alignmentSize = static_cast<unsigned>(TYPELESS_VECTOR_ALIGNMENT_SIZE);
	const unsigned slotPadding = (alignmentSize - (aTypeSize % alignmentSize)) % alignmentSize;
	return aTypeSize + slotPadding;
}
