#pragma once

//===============================================================================================
//Singleton
//===============================================================================================
template<typename T> class Singleton
{
public:

	//인스턴스 생성 및 초기화 
	static T* InitInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new T();
		}
		return _instance;
	}

	//인스턴스 반환
	static T* GetInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new T();
		}
		return _instance;
	}

	//인스턴스 제거
	static void CloseInstance()
	{
		if (_instance != nullptr)
		{
			delete _instance;
			_instance = nullptr;
		}
	}

	//인스턴스 제거
	static void Release()
	{
		CloseInstance(); 
	}

private:
	// 싱글톤 인스턴스
	static T* _instance;
};

template<typename T> T* Singleton<T>::_instance = nullptr;
