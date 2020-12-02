#pragma once

#include <cstddef>
#ifdef _WIN32
#include <guiddef.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CFPlugInCOM.h>
#else
#include "LinuxCOM.h"
#endif

template<typename T>
class com_ptr
{
	template<typename U>
		friend class com_ptr;

public:
	constexpr com_ptr();
	constexpr com_ptr(std::nullptr_t);
	explicit com_ptr(T* ptr);
	com_ptr(const com_ptr<T>& other);
	com_ptr(com_ptr<T>&& other);
	
	template<typename U>
	com_ptr(REFIID iid, com_ptr<U> other);

	~com_ptr();

	com_ptr<T>& operator=(std::nullptr_t);
	com_ptr<T>& operator=(T* ptr);
	com_ptr<T>& operator=(const com_ptr<T>& other);
	com_ptr<T>& operator=(com_ptr<T>&& other);

	T* get() const;
	T** releaseAndGetAddressOf();

	const T* operator->() const;
	T* operator->();
	const T& operator*() const;
	T& operator*();

	explicit operator bool() const;

	bool operator==(const com_ptr<T>& other) const;
	bool operator<(const com_ptr<T>& other) const;

private:
	void release();

	T* m_ptr;
};

template<typename T>
constexpr com_ptr<T>::com_ptr() :
	m_ptr(nullptr)
{ }

template<typename T>
constexpr com_ptr<T>::com_ptr(std::nullptr_t) :
	m_ptr(nullptr)
{ }

template<typename T>
com_ptr<T>::com_ptr(T* ptr) :
	m_ptr(ptr)
{
	if (m_ptr)
		m_ptr->AddRef();
}

template<typename T>
com_ptr<T>::com_ptr(const com_ptr<T>& other) :
	m_ptr(other.m_ptr)
{
	if (m_ptr)
		m_ptr->AddRef();
}

template<typename T>
com_ptr<T>::com_ptr(com_ptr<T>&& other) :
	m_ptr(other.m_ptr)
{
	other.m_ptr = nullptr;
}

template<typename T>
template<typename U>
com_ptr<T>::com_ptr(REFIID iid, com_ptr<U> other)
{
	if (other.m_ptr)
	{
		if (other.m_ptr->QueryInterface(iid, (void**)&m_ptr) != S_OK)
			m_ptr = nullptr;
	}
}

template<typename T>
com_ptr<T>::~com_ptr()
{
	release();
}

template<typename T>
com_ptr<T>& com_ptr<T>::operator=(std::nullptr_t)
{
	release();
	m_ptr = nullptr;
	return *this;
}

template<typename T>
com_ptr<T>& com_ptr<T>::operator=(T* ptr)
{
	if (ptr)
		ptr->AddRef();
	release();
	m_ptr = ptr;
	return *this;
}

template<typename T>
com_ptr<T>& com_ptr<T>::operator=(const com_ptr<T>& other)
{
	return (*this = other.m_ptr);
}

template<typename T>
com_ptr<T>& com_ptr<T>::operator=(com_ptr<T>&& other)
{
	release();
	m_ptr = other.m_ptr;
	other.m_ptr = nullptr;
	return *this;
}

template<typename T>
T* com_ptr<T>::get() const
{
	return m_ptr;
}

template<typename T>
T** com_ptr<T>::releaseAndGetAddressOf()
{
	release();
	return &m_ptr;
}

template<typename T>
const T* com_ptr<T>::operator->() const
{
	return m_ptr;
}

template<typename T>
T* com_ptr<T>::operator->()
{
	return m_ptr;
}

template<typename T>
const T& com_ptr<T>::operator*() const
{
	return *m_ptr;
}

template<typename T>
T& com_ptr<T>::operator*()
{
	return *m_ptr;
}

template<typename T>
com_ptr<T>::operator bool() const
{
	return m_ptr != nullptr;
}

template<typename T>
void com_ptr<T>::release()
{
	if (m_ptr)
		m_ptr->Release();
}

template<typename T>
bool com_ptr<T>::operator==(const com_ptr<T>& other) const
{
	return m_ptr == other.m_ptr;
}

template<typename T>
bool com_ptr<T>::operator<(const com_ptr<T>& other) const
{
	return m_ptr < other.m_ptr;
}

template<class T, class... Args>
com_ptr<T> make_com_ptr(Args&&... args)
{
	com_ptr<T> temp(new T(args...));
	// com_ptr takes ownership of reference count, so release reference count added by raw pointer constructor
	temp->Release();
	return std::move(temp);
}
