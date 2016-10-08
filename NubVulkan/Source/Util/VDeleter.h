#pragma once

#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <functional>

template<typename T>
class VDeleter
{
public:
	VDeleter();
	VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef);
	VDeleter(const VDeleter<VkInstance> &instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef);
	VDeleter(const VDeleter<VkDevice> &device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef);

	~VDeleter();

	const T *operator&() const;
	T *replace();
	operator T() const;
	void operator=(T rhs);
	
	template<typename V>
	bool operator==(V rhs);

protected:


private:
	T object{ VK_NULL_HANDLE };
	std::function<void(T)> deleter;

	void cleanup();

};

template<typename T>
inline VDeleter<T>::VDeleter() :
	VDeleter([](T, VkAllocationCallbacks*) {})
{
}

template<typename T>
inline VDeleter<T>::VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef)
{
	this->deleter = [=](T obj) {
		deletef(obj, nullptr);
	};
}

template<typename T>
inline VDeleter<T>::VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef)
{
	this->deleter = [&instance, deletef](T obj) {
		deletef(instance, obj, nullptr);
	};
}

template<typename T>
inline VDeleter<T>::~VDeleter()
{
	this->cleanup();
}

template<typename T>
inline const T * VDeleter<T>::operator&() const
{
	return &this->object;
}

template<typename T>
inline T * VDeleter<T>::replace()
{
	this->cleanup();
	return &this->object;
}

template<typename T>
inline VDeleter<T>::operator T() const
{
	return this->object;
}

template<typename T>
inline void VDeleter<T>::operator=(T rhs)
{
	if (rhs != this->object)
	{
		this->cleanup();
		this->object = rhs;
	}
}

template<typename T>
inline void VDeleter<T>::cleanup()
{
	if (this->object != VK_NULL_HANDLE)
	{
		deleter(this->object);
	}
	this->object = nullptr;
}

template<typename T>
template<typename V>
inline bool VDeleter<T>::operator==(V rhs)
{
	return object == T(rhs);
}
