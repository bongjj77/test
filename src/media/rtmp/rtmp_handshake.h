#pragma once
#include "rtmp_define.h"
#include <memory>
#include <vector>
#include <random>
#include <ctime>

//===============================================================================================
// RtmpHandshake
// -> C0 C1
// <- S0 S1 S2
// -> C2
//===============================================================================================
class RtmpHandshake {
public:
    RtmpHandshake() = default;
    ~RtmpHandshake() = default;

    static std::shared_ptr<std::vector<uint8_t>> MakeS0_S1_S2(const uint8_t *c1);
    static std::shared_ptr<std::vector<uint8_t>> MakeC0_C1();
    static std::shared_ptr<std::vector<uint8_t>> MakeC2(const uint8_t *s1);

private:
    static void FillHandshakePacket(std::vector<uint8_t>& buffer);
};