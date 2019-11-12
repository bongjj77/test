//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#include "rtmp_handshake.h"
#include <random>
#ifdef WIN32
#include <openssl/sha.h>
#include <openssl/hmac.h>
#else
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#endif 

//
std::vector<uint8_t> RtmpHandshake::_fms_key_table = // 36+32
{
	0x47, 0x65, 0x6e, 0x75, 0x69, 0x6e, 0x65, 0x20, 0x41, 0x64, 0x6f, 0x62, 0x65, 0x20, 0x46, 0x6c,
	0x61, 0x73, 0x68, 0x20, 0x4d, 0x65, 0x64, 0x69, 0x61, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72,
	0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Media Server 001 

	0xf0, 0xee, 0xc2, 0x4a, 0x80, 0x68, 0xbe, 0xe8, 0x2e, 0x00, 0xd0, 0xd1, 0x02, 0x9e, 0x7e, 0x57,
	0x6e, 0xec, 0x5d, 0x2d, 0x29, 0x80, 0x6f, 0xab, 0x93, 0xb8, 0xe6, 0x36, 0xcf, 0xeb, 0x31, 0xae,
}; // 68

std::vector<uint8_t> RtmpHandshake::_fp_key_table =
{
	0x47, 0x65, 0x6E, 0x75, 0x69, 0x6E, 0x65, 0x20, 0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x46, 0x6C,
	0x61, 0x73, 0x68, 0x20, 0x50, 0x6C, 0x61, 0x79, 0x65, 0x72, 0x20, 0x30, 0x30, 0x31, 0xF0, 0xEE,
	0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02, 0x9E, 0x7E, 0x57, 0x6E, 0xEC,
	0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
}; // 62

//====================================================================================================
// Make C1 
//====================================================================================================
bool RtmpHandshake::MakeC1(uint8_t * sig)
{
	std::mt19937 random_number;
 
	if( !sig ) 
	{ 
		return false;
	}

	memset(sig, 0, RTMP_HANDSHAKE_PACKET_SIZE);
	*(uint32_t *)&sig[0] = htonl( (uint32_t )time(nullptr) );//htonl( GetCurrentTick() );
	*(uint32_t *)&sig[4] = 0x01020503; // 03 05 02 01

	for(int index = 8; index < RTMP_HANDSHAKE_PACKET_SIZE; index++)
	{ 
		sig[index] = (uint8_t)(random_number() % 256);
	}
 
	int offset = GetDigestOffset1(sig);
	 
	CalculateDigest(offset, sig, (uint8_t*)_fms_key_table.data(), 36, sig+offset);

	return true;
}

//====================================================================================================
// Make S1
//====================================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpHandshake::MakeS1()
{ 
	auto buffer = std::make_shared<std::vector<uint8_t>>(RTMP_HANDSHAKE_PACKET_SIZE);
	uint8_t * data_pos = buffer->data();

	std::mt19937 random_number;
 
	*(uint32_t *)&data_pos[0] = htonl( (uint32_t )time(nullptr) );
	*(uint32_t *)&data_pos[4] = 0x01020503; // 03 05 02 01
	
	for(int index = 8; index < RTMP_HANDSHAKE_PACKET_SIZE; index++)
	{ 
		data_pos[index] = (uint8_t)(random_number() % 256);
	}
 
	int offset = GetDigestOffset1(data_pos);
	 
	CalculateDigest(offset, data_pos, (uint8_t*)_fms_key_table.data(), 36, data_pos + offset);

	return buffer;
}

//===============================================================================================
// Make S2
//===============================================================================================
std::shared_ptr<std::vector<uint8_t>> RtmpHandshake::MakeS2(uint8_t * client_data)
{
	auto buffer = std::make_shared<std::vector<uint8_t>>(RTMP_HANDSHAKE_PACKET_SIZE);
	uint8_t * data_pos = buffer->data();

	std::mt19937 random_number;
	uint8_t	digest[SHA256_DIGEST_LENGTH];
 
	// version value 0 echo
	if( *(uint32_t *)&client_data[4] == 0 )
	{
		memcpy(data_pos, client_data, RTMP_HANDSHAKE_PACKET_SIZE);
		return buffer;
	}

	// client digest offset 구하기
	int offset = GetDigestOffset1(client_data);
	if( !VerifyDigest(offset, client_data, (uint8_t*)_fp_key_table.data(), 30) )
	{
		offset = GetDigestOffset2(client_data);
		if( !VerifyDigest(offset, client_data, (uint8_t*)_fp_key_table.data(), 30) )
		{
			memcpy(data_pos, client_data, RTMP_HANDSHAKE_PACKET_SIZE);
			return buffer;
		}
	}

	// client digest 구하기
	HmacSha256(client_data + offset, SHA256_DIGEST_LENGTH, (uint8_t*)_fms_key_table.data(), _fms_key_table.size(), digest);

	// s2 만들기
	for(int index = 0; index < 1504; index++)
	{ 
		data_pos[index] = (uint8_t)(random_number() % 256);
	}

	HmacSha256(data_pos, 1504, digest, SHA256_DIGEST_LENGTH, data_pos + 1504);

	return buffer;
}

//===============================================================================================
// Make C2
//===============================================================================================
bool RtmpHandshake::MakeC2(uint8_t * client_sig, uint8_t * sig)
{
	uint8_t	svrDigest[SHA256_DIGEST_LENGTH];
  
	if( !client_sig || !sig )
	{ 
		return false; 
	}

    // copy
	//memcpy(sig, client_sig, RTMP_HANDSHAKE_PACKET_SIZE);

    int offset = GetDigestOffset1(client_sig);
    CalculateDigest(offset, client_sig, (uint8_t*)_fms_key_table.data(), 36, svrDigest);

    if (memcmp(client_sig+offset, svrDigest, SHA256_DIGEST_LENGTH)) 
	{
    	return false;
    } 
	
    memcpy(sig, client_sig, RTMP_HANDSHAKE_PACKET_SIZE);
	
    sig[4] = sig[5] = sig[6] = sig[7] = 0;

	return true;
}

//===============================================================================================
// GetDigestOffset1
//===============================================================================================
int RtmpHandshake::GetDigestOffset1(uint8_t * handshake)
{
	uint8_t	*ptr = handshake + 8;
	int		offset = 0;

	offset += (*ptr); ptr++;
	offset += (*ptr); ptr++;
	offset += (*ptr); ptr++;
	offset += (*ptr);

	return (offset % 728) + 12;
}

//===============================================================================================
// GetDigestOffset2
//===============================================================================================
int RtmpHandshake::GetDigestOffset2(uint8_t * handshake)
{
	uint8_t	*ptr = handshake + 772;
	int		offset = 0;

	offset += (*ptr); ptr++;
	offset += (*ptr); ptr++;
	offset += (*ptr); ptr++;
	offset += (*ptr);

	return (offset % 728) + 776;
}

//===============================================================================================
// HmacSha256
//===============================================================================================
void RtmpHandshake::HmacSha256(uint8_t *message, int message_size, uint8_t *key, int key_size, uint8_t *digest)
{
	uint32_t result_size = 0;
	
	/*
	HMAC_CTX	ctx;
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, key, key_size, EVP_sha256(), nullptr);
	HMAC_Update(&ctx, message, message_size);
	HMAC_Final(&ctx, digest, &digest_len);
	HMAC_CTX_cleanup(&ctx);	
	*/
	HMAC(EVP_sha256(), key, key_size,  message, message_size, digest, &result_size);

}

//===============================================================================================
// CalculateDigest
//===============================================================================================
void RtmpHandshake::CalculateDigest(int digest_pos, uint8_t * message, uint8_t *key, int key_size, uint8_t *digest)
{
	const int	buffer_size = RTMP_HANDSHAKE_PACKET_SIZE - SHA256_DIGEST_LENGTH;
	uint8_t		buffer[buffer_size] = {0,};

	memcpy(buffer, message, (size_t)digest_pos);
	memcpy(buffer + digest_pos, &message[digest_pos+SHA256_DIGEST_LENGTH], (size_t)(buffer_size - digest_pos));

	HmacSha256(buffer, buffer_size, key, key_size, digest);
}

//===============================================================================================
// VerifyDigest
//===============================================================================================
bool RtmpHandshake::VerifyDigest(int digest_pos, uint8_t *message, uint8_t *key, int key_size)
{
	uint8_t	calc_digest[SHA256_DIGEST_LENGTH];

	CalculateDigest(digest_pos, message, key, key_size, calc_digest);

    return !memcmp(message + digest_pos, calc_digest, SHA256_DIGEST_LENGTH);
}



