#ifndef __MD_5_H__
#define __MD_5_H__

#include <stdint.h>
#include <string>

namespace xwb{

typedef struct {
	uint32_t state[4];
	uint32_t count[4];
	uint8_t buffer[64];
}MD5_CTX;


void MD5Init(MD5_CTX *ctx);
void MD5Update(MD5_CTX *ctx, uint8_t *pBuffer, uint32_t len);
void MD5Final(MD5_CTX *ctx, uint8_t hash[16]);


void MD5Digest(const uint8_t *pBuffer, uint32_t len, uint8_t digest[16]);
void MD5Digest(const int8_t *pBuffer, uint32_t len, uint8_t digest[16]);
std::string MD5Digest(const uint8_t *pBuffer, uint32_t len);


} // namespace xwb

#endif
