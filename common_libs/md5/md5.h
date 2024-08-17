/*********************************************************************
* Filename:   md5.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding MD5 implementation.
*********************************************************************/

#ifndef MD5_H
#define MD5_H

#ifdef __cplusplus
extern "C" {
#endif

/*************************** HEADER FILES ***************************/
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

/****************************** MACROS ******************************/
#define MD5_BLOCK_SIZE 16               // MD5 outputs a 16 byte digest

/**************************** DATA TYPES ****************************/
typedef uint8_t  BYTE;
typedef uint32_t WORD;

typedef struct {
   BYTE data[64];
   WORD datalen;
   unsigned long long bitlen;
   WORD state[4];
} MD5_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void md5_init(MD5_CTX *ctx);
void md5_update(MD5_CTX *ctx, const BYTE data[], size_t len);
void md5_final(MD5_CTX *ctx, BYTE hash[]);
int  md5_file(size_t (*read)(void *buf, size_t len, size_t n, void *file),
              void *file, BYTE *buffer, size_t buffer_len, size_t max_file_size, BYTE hash[]);

#ifdef __cplusplus
}
#endif

#endif   // MD5_H
