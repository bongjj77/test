//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include "rtmp_define.h"


//===============================================================================================
// RtmpHandshake
//===============================================================================================
class RtmpHandshake
{
public:
	RtmpHandshake() = default;
	~RtmpHandshake() = default;

public:
	static	std::shared_ptr<std::vector<uint8_t>> MakeS1();
	static	bool MakeC1(uint8_t *sig);
	
	static	std::shared_ptr<std::vector<uint8_t>> MakeS2(uint8_t *client_data);
    static	bool MakeC2(uint8_t *client_sig, uint8_t *sig);

private:
	static	int GetDigestOffset1(uint8_t *handshake);
	static	int GetDigestOffset2(uint8_t *handshake);
	static	void HmacSha256(uint8_t *message, int message_size, uint8_t *key, int key_size, uint8_t *digest);
	static	void CalculateDigest(int digest_pos, uint8_t *message, uint8_t *key, int key_size, uint8_t *digest);
	static	bool VerifyDigest(int digest_pos, uint8_t *message, uint8_t *key, int key_size);
	

protected : 
	static std::vector<uint8_t> _fms_key_table;
	static std::vector<uint8_t> _fp_key_table;
};

