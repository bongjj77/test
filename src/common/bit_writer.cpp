#include "bit_writer.h"
#include <cstring>
#include <stdexcept>

//====================================================================================================
// BitWriter 생성자: 데이터 크기와 예약 크기를 인자로 받음
//====================================================================================================
BitWriter::BitWriter(uint32_t dataSize, uint32_t reserveSize)
    : _data(std::make_shared<std::vector<uint8_t>>(dataSize + reserveSize)), _dataPos(_data->data()),
      _maxBitCount((dataSize + reserveSize) * 8) {
  std::memset(_data->data(), 0, _data->size());
}

//====================================================================================================
// BitWriter 생성자: 데이터 크기와 데이터 포인터를 인자로 받음
//====================================================================================================
BitWriter::BitWriter(uint32_t dataSize, uint8_t *dataPos)
    : _data(nullptr), _dataPos(dataPos), _maxBitCount(dataSize * 8) {}

//====================================================================================================
// 비트를 기록하는 함수
//====================================================================================================
void BitWriter::Write(uint32_t bitCount, uint32_t value) {
  if (_bitCount + bitCount > _maxBitCount) {
    throw std::overflow_error("BitWriter overflow: exceeding max bit count.");
  }

  for (uint32_t i = 0; i < bitCount; ++i) {
    uint32_t bit = (value >> (bitCount - 1 - i)) & 1; // MSB부터 기록
    uint32_t byteOffset = _bitCount / 8;
    uint32_t bitOffset = _bitCount % 8;
    _dataPos[byteOffset] |= (bit << (7 - bitOffset)); // MSB부터 기록
    ++_bitCount;
  }
}
