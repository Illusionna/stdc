#ifndef _MD5_H_
#define _MD5_H_


#include <stdio.h>
#include <string.h>


#define F(x, y, z) ((x & y) | (~x & z))
#define G(x, y, z) ((x & z) | (y & ~z))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))
#define ROTATE_LEFT(x, n) ((x << n) | (x >> (32 - n)))
#define FF(a, b, c, d, x, s, ac) {a = a + F(b, c, d) + x + ac; a = ROTATE_LEFT(a, s); a = a + b;}
#define GG(a, b, c, d, x, s, ac) {a = a + G(b, c, d) + x + ac; a = ROTATE_LEFT(a, s); a = a + b;}
#define HH(a, b, c, d, x, s, ac) {a = a + H(b, c, d) + x + ac; a = ROTATE_LEFT(a, s); a = a + b;}
#define II(a, b, c, d, x, s, ac) {a = a + I(b, c, d) + x + ac; a = ROTATE_LEFT(a, s); a = a + b;}


typedef struct{
    unsigned int count[2];
	unsigned int state[4];
	unsigned char buffer[64];
} _ContextMD5;


void __md5_init__(_ContextMD5 *context);


void __md5_update__(_ContextMD5 *context, unsigned char *input, unsigned int length);


void __md5_finalize__(_ContextMD5 *context, unsigned char *digest);


void __md5_transform__(unsigned int *state, unsigned char *block);


void __md5_encode__(unsigned char *output, unsigned int *input, unsigned int length);


void __md5_decode__(unsigned int *output, unsigned char *input, unsigned int length);


void md5_string(char *input, char *hash);


void md5_file(FILE *file, char *hash);


#endif