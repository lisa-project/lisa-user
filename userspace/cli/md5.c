#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "md5.h"

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))


#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define ROUND1(a, b, c, d, x, s, ac) { \
    (a) += F ((b), (c), (d)) + (x) + (unsigned int)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
  }
#define ROUND2(a, b, c, d, x, s, ac) { \
    (a) += G ((b), (c), (d)) + (x) + (unsigned int)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
  }
#define ROUND3(a, b, c, d, x, s, ac) { \
    (a) += H ((b), (c), (d)) + (x) + (unsigned int)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
  }
#define ROUND4(a, b, c, d, x, s, ac) { \
    (a) += I ((b), (c), (d)) + (x) + (unsigned int)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
  }

struct md5_context {
  unsigned int abcd[4];
  unsigned char digest[16];
};

void print(unsigned char *data, int length)
{
  int i; 
  for (i = 0; i<length; i++) {
    printf("%02x ", data[i]);
  }
  printf("\n");
}


void encode(unsigned int*  abcd, unsigned char* res, int length)
{
  int i = 0, j = 0;
  for (j = 0; j<length; j += 4)
  {
    res[j] = abcd[i] & 0xFF;
    res[j+1] = (abcd[i] >> 8) & 0xFF;
    res[j+2] = (abcd[i] >> 16) & 0xFF;
    res[j+3] = (abcd[i] >> 24) & 0xFF;

    i++;
  }
}

void init_context(struct md5_context *context)
{
  context->abcd[0] = 0x67452301;  
  context->abcd[1] = 0xefcdab89;
  context->abcd[2] = 0x98badcfe;
  context->abcd[3] = 0x10325476;
}


/*
 * Copy the length of the original text in last 8 octets
 * The length is in little endian order
 */
void set_length(unsigned char *data, int l, unsigned int* length) {
  unsigned char dim[8];
  unsigned int lg = *length;
  unsigned int bit_count[2];//size of original message in bits
  
  memset(data + lg - 8, 0, 8);
  memset(bit_count, 0, 2 * sizeof(unsigned int));

  bit_count[0] = l << 3;
  encode(bit_count, dim, 8);

  memcpy(data + lg, dim, 8);

  *length += 8;
}

void transform(struct md5_context *context, unsigned char *data, int length) 
{

  unsigned int a, b, c, d;
  unsigned int x[16];
  int i, j;

  


  for (i = 0, j = 0; j < length; i++, j += 4)
  {
    x[i] = ((unsigned int)data[j]) | (((unsigned int)data[j+1]) << 8) |
      (((unsigned int)data[j+2]) << 16) | (((unsigned int)data[j+3]) << 24);
  }


  a = context->abcd[0];
  b = context->abcd[1];
  c = context->abcd[2];
  d = context->abcd[3];
  
  // Round 1
  ROUND1(a, b, c, d, x[ 0], S11, 0xd76aa478);
  ROUND1(d, a, b, c, x[ 1], S12, 0xe8c7b756);
  ROUND1(c, d, a, b, x[ 2], S13, 0x242070db);
  ROUND1(b, c, d, a, x[ 3], S14, 0xc1bdceee);
  ROUND1(a, b, c, d, x[ 4], S11, 0xf57c0faf);
  ROUND1(d, a, b, c, x[ 5], S12, 0x4787c62a);
  ROUND1(c, d, a, b, x[ 6], S13, 0xa8304613);
  ROUND1(b, c, d, a, x[ 7], S14, 0xfd469501);
  ROUND1(a, b, c, d, x[ 8], S11, 0x698098d8);
  ROUND1(d, a, b, c, x[ 9], S12, 0x8b44f7af);
  ROUND1(c, d, a, b, x[10], S13, 0xffff5bb1);
  ROUND1(b, c, d, a, x[11], S14, 0x895cd7be);
  ROUND1(a, b, c, d, x[12], S11, 0x6b901122);
  ROUND1(d, a, b, c, x[13], S12, 0xfd987193);
  ROUND1(c, d, a, b, x[14], S13, 0xa679438e);
  ROUND1(b, c, d, a, x[15], S14, 0x49b40821);
  
 // Round 2
  ROUND2(a, b, c, d, x[ 1], S21, 0xf61e2562);
  ROUND2(d, a, b, c, x[ 6], S22, 0xc040b340);
  ROUND2(c, d, a, b, x[11], S23, 0x265e5a51);
  ROUND2(b, c, d, a, x[ 0], S24, 0xe9b6c7aa);
  ROUND2(a, b, c, d, x[ 5], S21, 0xd62f105d);
  ROUND2(d, a, b, c, x[10], S22,  0x2441453);
  ROUND2(c, d, a, b, x[15], S23, 0xd8a1e681);
  ROUND2(b, c, d, a, x[ 4], S24, 0xe7d3fbc8);
  ROUND2(a, b, c, d, x[ 9], S21, 0x21e1cde6);
  ROUND2(d, a, b, c, x[14], S22, 0xc33707d6);
  ROUND2(c, d, a, b, x[ 3], S23, 0xf4d50d87);
  ROUND2(b, c, d, a, x[ 8], S24, 0x455a14ed);
  ROUND2(a, b, c, d, x[13], S21, 0xa9e3e905);
  ROUND2(d, a, b, c, x[ 2], S22, 0xfcefa3f8);
  ROUND2(c, d, a, b, x[ 7], S23, 0x676f02d9);
  ROUND2(b, c, d, a, x[12], S24, 0x8d2a4c8a);

  // Round 3
  ROUND3(a, b, c, d, x[ 5], S31, 0xfffa3942);
  ROUND3(d, a, b, c, x[ 8], S32, 0x8771f681);
  ROUND3(c, d, a, b, x[11], S33, 0x6d9d6122);
  ROUND3(b, c, d, a, x[14], S34, 0xfde5380c);
  ROUND3(a, b, c, d, x[ 1], S31, 0xa4beea44);
  ROUND3(d, a, b, c, x[ 4], S32, 0x4bdecfa9);
  ROUND3(c, d, a, b, x[ 7], S33, 0xf6bb4b60);
  ROUND3(b, c, d, a, x[10], S34, 0xbebfbc70);
  ROUND3(a, b, c, d, x[13], S31, 0x289b7ec6);
  ROUND3(d, a, b, c, x[ 0], S32, 0xeaa127fa);
  ROUND3(c, d, a, b, x[ 3], S33, 0xd4ef3085);
  ROUND3(b, c, d, a, x[ 6], S34,  0x4881d05);
  ROUND3(a, b, c, d, x[ 9], S31, 0xd9d4d039);
  ROUND3(d, a, b, c, x[12], S32, 0xe6db99e5);
  ROUND3(c, d, a, b, x[15], S33, 0x1fa27cf8);
  ROUND3(b, c, d, a, x[ 2], S34, 0xc4ac5665);

  // Round 4
  ROUND4(a, b, c, d, x[ 0], S41, 0xf4292244);
  ROUND4(d, a, b, c, x[ 7], S42, 0x432aff97);
  ROUND4(c, d, a, b, x[14], S43, 0xab9423a7);
  ROUND4(b, c, d, a, x[ 5], S44, 0xfc93a039);
  ROUND4(a, b, c, d, x[12], S41, 0x655b59c3);
  ROUND4(d, a, b, c, x[ 3], S42, 0x8f0ccc92);
  ROUND4(c, d, a, b, x[10], S43, 0xffeff47d);
  ROUND4(b, c, d, a, x[ 1], S44, 0x85845dd1);
  ROUND4(a, b, c, d, x[ 8], S41, 0x6fa87e4f);
  ROUND4(d, a, b, c, x[15], S42, 0xfe2ce6e0);
  ROUND4(c, d, a, b, x[ 6], S43, 0xa3014314);
  ROUND4(b, c, d, a, x[13], S44, 0x4e0811a1);
  ROUND4(a, b, c, d, x[ 4], S41, 0xf7537e82);
  ROUND4(d, a, b, c, x[11], S42, 0xbd3af235);
  ROUND4(c, d, a, b, x[ 2], S43, 0x2ad7d2bb);
  ROUND4(b, c, d, a, x[ 9], S44, 0xeb86d391);
  
  context->abcd[0] += a;
  context->abcd[1] += b;
  context->abcd[2] += c;
  context->abcd[3] += d;

  memset(x, 0, sizeof(x));
}


void get_digest(struct md5_context* context, char *data, unsigned int length)
{
  int no_blocks;
  int i;

  /* 
   *  Get number of blocks of 512 bits (64 octets)
   *  The data buffer was padded so it n*64 octets  
   */
  no_blocks = length >> 6;
 
  for (i = 0; i<no_blocks; i++) 
  {
    transform(context, data + i*64, 64);
  }

  /* Get digest */
  encode(context->abcd, context->digest, 16);

  //print(context->digest, 16);

}

unsigned char*  md5(char *msg)
{
  unsigned char* data;
  unsigned int length, msg_l, pad_l;
  struct md5_context context;
  unsigned char* digest;

  digest = (char*) calloc(16, sizeof(unsigned char));

  /* Add padding to original data */
  msg_l = strlen(msg);
  pad_l = msg_l % 64 ;
  if (pad_l < 56) 
    pad_l = 56 - pad_l;
  else 
    pad_l = 120 - pad_l;
 
  length = pad_l + msg_l;

  /* Alloc enougth space to append 8 octets for length */
  data = (char*) calloc(length + 8, sizeof(char));
  memcpy(data, msg, msg_l);
  data[msg_l] = 0x80;
  memset(data + msg_l +1, 0, pad_l - 1);

  /* Add length of original data */
  set_length(data, msg_l, &length);

  /* Init */
  init_context(&context);

  get_digest(&context, data, length);

  print(context.digest, 16);
  memcpy(digest, context.digest, 16);

  free(data);

  return digest;
}


