#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

//====================================================================================================
// BitWriter
//====================================================================================================
class BitWriter {
public:
  // 생성자: 데이터 크기와 예약 크기를 인자로 받음
  explicit BitWriter(uint32_t dataSize, uint32_t reserveSize = 0);

  // 생성자: 데이터 크기와 데이터 포인터를 인자로 받음
  BitWriter(uint32_t dataSize, uint8_t *dataPos);

  // 소멸자
  ~BitWriter() = default;

  // 비트를 기록하는 함수
  void Write(uint32_t bitCount, uint32_t value);

  // 데이터 벡터를 반환하는 함수
  std::shared_ptr<std::vector<uint8_t>> GetData() const { return _data; }

private:
  std::shared_ptr<std::vector<uint8_t>> _data = nullptr; // 데이터 벡터
  uint8_t *_dataPos = nullptr;                           // 현재 데이터 위치
  uint32_t _bitCount = 0;                                // 현재까지 기록된 비트 수
  uint32_t _maxBitCount = 0;                             // 최대 비트 수
};
