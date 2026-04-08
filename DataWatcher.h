#pragma once
#include "tge/WASD/Util/TypeTraits.h"

#include <tuple>
#include <type_traits>

namespace WASD
{
	template<typename T>
	class DataWatcher
	{
	public:
		explicit DataWatcher(T&& aData);
		explicit DataWatcher(const T& aData);
		template<isTuple Tuple>
		explicit DataWatcher(Tuple&& aTuple);
		template<isTuple Tuple>
		explicit DataWatcher(const Tuple& aTuple);
		DataWatcher() = default;
		DataWatcher(DataWatcher&& aOther) noexcept;
		DataWatcher(const DataWatcher& aOther);
		DataWatcher& operator=(DataWatcher&& aOther) noexcept;
		DataWatcher& operator=(const DataWatcher& aOther);
		DataWatcher& operator=(T&& aData) noexcept;
		DataWatcher& operator=(const T& aData);

		~DataWatcher() = default;

		[[nodiscard]] T& Write();
		[[nodiscard]] const T& Read() const;

		DataWatcher& CleanCopy(DataWatcher&& aOther);
		DataWatcher& CleanCopy(const DataWatcher& aOther);
		DataWatcher& CleanCopy(T&& aData);
		DataWatcher& CleanCopy(const T& aData);

		[[nodiscard]] bool IsDirty() const;
		void Clean() const;
		void MakeDirty() const;

		[[nodiscard]] bool operator==(const DataWatcher& aOther) const;
		[[nodiscard]] bool operator==(const T& aData) const;

	private:
		template<typename Tuple, std::size_t ... N>
		DataWatcher(Tuple&& aTuple, std::index_sequence<N...>);
		template<typename Tuple, std::size_t ... N>
		DataWatcher(const Tuple& aTuple, std::index_sequence<N...>);

		T myData               = {};
		mutable bool myIsDirty = true;
	};
}

template<typename T>
WASD::DataWatcher<T>::DataWatcher(T&& aData) :
	myData(std::move(aData))
{
}

template<typename T>
WASD::DataWatcher<T>::DataWatcher(const T& aData) :
	myData(aData)
{
}

template<typename T>
template<WASD::isTuple Tuple>
WASD::DataWatcher<T>::DataWatcher(Tuple&& aTuple) :
	DataWatcher(std::forward<Tuple>(aTuple), MakeTupleIndexSequence<Tuple>{})
{
}

template<typename T>
template<WASD::isTuple Tuple>
WASD::DataWatcher<T>::DataWatcher(const Tuple& aTuple) :
	DataWatcher(std::forward<Tuple>(aTuple), MakeTupleIndexSequence<Tuple>{})
{
}

template<typename T>
WASD::DataWatcher<T>::DataWatcher(DataWatcher&& aOther) noexcept :
	myData(std::move(aOther.myData))
{
	aOther.myData    = {};
	aOther.myIsDirty = true;
}

template<typename T>
WASD::DataWatcher<T>::DataWatcher(const DataWatcher& aOther) :
	myData(aOther.myData)
{
}

template<typename T>
WASD::DataWatcher<T>& WASD::DataWatcher<T>::operator=(DataWatcher&& aOther) noexcept
{
	if (this == &aOther)
	{
		return *this;
	}

	myData           = std::move(aOther.myData);
	myIsDirty        = true;
	aOther.myData    = {};
	aOther.myIsDirty = true;
	return *this;
}

template<typename T>
WASD::DataWatcher<T>& WASD::DataWatcher<T>::operator=(const DataWatcher& aOther)
{
	if (this == &aOther)
	{
		return *this;
	}

	myData    = aOther.myData;
	myIsDirty = true;

	return *this;
}

template<typename T>
WASD::DataWatcher<T>& WASD::DataWatcher<T>::operator=(T&& aData) noexcept
{
	if (&myData == &aData)
	{
		return *this;
	}

	myData    = std::move(aData);
	myIsDirty = true;
	aData     = {};

	return *this;
}

template<typename T>
WASD::DataWatcher<T>& WASD::DataWatcher<T>::operator=(const T& aData)
{
	if (&myData == &aData)
	{
		return *this;
	}

	myData    = aData;
	myIsDirty = true;

	return *this;
}

template<typename T>
T& WASD::DataWatcher<T>::Write()
{
	myIsDirty = true;
	return myData;
}

template<typename T>
const T& WASD::DataWatcher<T>::Read() const
{
	return myData;
}

template<typename T>
WASD::DataWatcher<T>& WASD::DataWatcher<T>::CleanCopy(DataWatcher&& aOther)
{
	myData           = std::move(aOther.myData);
	aOther.myData    = {};
	aOther.myIsDirty = true;
	return *this;
}

template<typename T>
WASD::DataWatcher<T>& WASD::DataWatcher<T>::CleanCopy(const DataWatcher& aOther)
{
	if (this == aOther)
	{
		return *this;
	}

	myData = aOther.myData;
	return *this;
}

template<typename T>
WASD::DataWatcher<T>& WASD::DataWatcher<T>::CleanCopy(T&& aData)
{
	myData = std::move(aData);
	return *this;
}

template<typename T>
WASD::DataWatcher<T>& WASD::DataWatcher<T>::CleanCopy(const T& aData)
{
	myData = aData;
	return *this;
}

template<typename T>
bool WASD::DataWatcher<T>::IsDirty() const
{
	return myIsDirty;
}

template<typename T>
void WASD::DataWatcher<T>::Clean() const
{
	myIsDirty = false;
}

template<typename T>
void WASD::DataWatcher<T>::MakeDirty() const
{
	myIsDirty = true;
}

template<typename T>
bool WASD::DataWatcher<T>::operator==(const DataWatcher& aOther) const
{
	return myData == aOther.myData;
}

template<typename T>
bool WASD::DataWatcher<T>::operator==(const T& aData) const
{
	return myData == aData;
}

template<typename T>
template<typename Tuple, std::size_t... N>
WASD::DataWatcher<T>::DataWatcher(Tuple&& aTuple, std::index_sequence<N...>) :
	myData(std::get<N>(std::forward<Tuple>(aTuple))...)
{
}

template<typename T>
template<typename Tuple, std::size_t... N>
WASD::DataWatcher<T>::DataWatcher(const Tuple& aTuple, std::index_sequence<N...>) :
	myData(std::get<N>(std::forward<Tuple>(aTuple))...)
{
}
