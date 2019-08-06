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
		if (m_pInstance == nullptr)
		{
			m_pInstance = new T();
		}
		return m_pInstance;
	}

	//인스턴스 반환
	static T* GetInstance()
	{
		if (m_pInstance == nullptr)
		{
			m_pInstance = new T();
		}
		return m_pInstance;
	}

	//인스턴스 제거
	static void CloseInstance()
	{
		if (m_pInstance != nullptr)
		{
			delete m_pInstance;
			m_pInstance = nullptr;
		}
	}

	//인스턴스 제거
	static void Release()
	{
		CloseInstance(); 
	}

private:
	// 싱글톤 인스턴스
	static T* m_pInstance;
};

template<typename T> T* Singleton<T>::m_pInstance = nullptr;
