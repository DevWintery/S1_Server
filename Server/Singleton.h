#pragma once

template <typename T>
class Singleton
{
public:
	static T* GetInstance()
	{
		std::call_once(flag, []() {
			instance.reset(new T);
			});
		return instance.get();
	}

	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(Singleton&&) = delete;

protected:
	Singleton() {}
	~Singleton() {}

private:
	static std::unique_ptr<T> instance;
	static std::once_flag flag;
};

template <typename T>
std::unique_ptr<T> Singleton<T>::instance = nullptr;

template <typename T>
std::once_flag Singleton<T>::flag;