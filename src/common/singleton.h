#pragma once
#include <memory>
#include <mutex>

//===============================================================================================
// Singleton
//===============================================================================================
template <typename T> class Singleton {
public:
  // 인스턴스 생성 및 초기화
  static T *InitInstance() {
    std::call_once(initFlag, []() { _instance.reset(new T()); });
    return _instance.get();
  }

  // 인스턴스 반환
  static T *GetInstance() { return InitInstance(); }

  // 인스턴스 제거
  static void CloseInstance() { _instance.reset(nullptr); }

  // 인스턴스 제거 (호환성 유지)
  static void Release() { CloseInstance(); }

protected:
  Singleton() = default;
  virtual ~Singleton() = default;

private:
  Singleton(const Singleton &) = delete;
  Singleton &operator=(const Singleton &) = delete;

  // 싱글톤 인스턴스
  static std::unique_ptr<T> _instance;
  static std::once_flag initFlag;
};

template <typename T> std::unique_ptr<T> Singleton<T>::_instance = nullptr;
template <typename T> std::once_flag Singleton<T>::initFlag;
