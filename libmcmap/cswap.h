//Thanks to Todd Markley for helping write this, based on GNU's <byteswap.h>

#ifndef CSWAPH
#define CSWAPH 1

#include <stdint.h>

#define cswapr_16(x) (((((uint16_t)(((uint8_t*)(x))[1]))<<0)&0x00ff) + \
                     ((((uint16_t)(((uint8_t*)(x))[0]))<<8)&0xff00))

#define cswapw_16(x,i) { \
	((uint8_t*)(x))[0] = (uint8_t)((((uint16_t)(i))>>8)&0x00ff); \
	((uint8_t*)(x))[1] = (uint8_t)((((uint16_t)(i))>>0)&0x00ff); \
	}

#define cswapr_24(x) (((((uint32_t)(((uint8_t*)(x))[2]))<<0)&0x0000ff) + \
                     ((((uint32_t)(((uint8_t*)(x))[1]))<<8)&0x00ff00) + \
                     ((((uint32_t)(((uint8_t*)(x))[0]))<<16)&0xff0000))

#define cswapw_24(x,i) { \
	((uint8_t*)(x))[0] = (uint8_t)((((uint32_t)(i))>>16)&0x0000ff); \
	((uint8_t*)(x))[1] = (uint8_t)((((uint32_t)(i))>>8)&0x0000ff); \
	((uint8_t*)(x))[2] = (uint8_t)((((uint32_t)(i))>>0)&0x0000ff); \
	}

#define cswapr_32(x) (((((uint32_t)(((uint8_t*)(x))[3]))<<0)&0x000000ff) + \
                    ((((uint32_t)(((uint8_t*)(x))[2]))<<8)&0x0000ff00) + \
                    ((((uint32_t)(((uint8_t*)(x))[1]))<<16)&0x00ff0000) + \
                    ((((uint32_t)(((uint8_t*)(x))[0]))<<24)&0xff000000))

#define cswapw_32(x,i) { \
	((uint8_t*)(x))[0] = (uint8_t)((((uint32_t)(i))>>24)&0x000000ff); \
	((uint8_t*)(x))[1] = (uint8_t)((((uint32_t)(i))>>16)&0x000000ff); \
	((uint8_t*)(x))[2] = (uint8_t)((((uint32_t)(i))>>8)&0x000000ff); \
	((uint8_t*)(x))[3] = (uint8_t)((((uint32_t)(i))>>0)&0x000000ff); \
	}

#define cswapr_64(x) \
      (((((uint64_t)(((uint8_t*)(x))[7]))<<0)&0x00000000000000ffull) + \
      ((((uint64_t)(((uint8_t*)(x))[6]))<<8)&0x000000000000ff00ull) + \
      ((((uint64_t)(((uint8_t*)(x))[5]))<<16)&0x0000000000ff0000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[4]))<<24)&0x00000000ff000000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[3]))<<32)&0x000000ff00000000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[2]))<<40)&0x0000ff0000000000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[1]))<<48)&0x00ff000000000000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[0]))<<56)&0xff00000000000000ull))

#define cswapw_64(x,i) { \
	((uint8_t*)(x))[0] = (uint8_t)((((uint64_t)(i))>>56)&0x00000000000000ffull); \
	((uint8_t*)(x))[1] = (uint8_t)((((uint64_t)(i))>>48)&0x00000000000000ffull); \
	((uint8_t*)(x))[2] = (uint8_t)((((uint64_t)(i))>>40)&0x00000000000000ffull); \
	((uint8_t*)(x))[3] = (uint8_t)((((uint64_t)(i))>>32)&0x00000000000000ffull); \
	((uint8_t*)(x))[4] = (uint8_t)((((uint64_t)(i))>>24)&0x00000000000000ffull); \
	((uint8_t*)(x))[5] = (uint8_t)((((uint64_t)(i))>>16)&0x00000000000000ffull); \
	((uint8_t*)(x))[6] = (uint8_t)((((uint64_t)(i))>>8)&0x00000000000000ffull); \
	((uint8_t*)(x))[7] = (uint8_t)((((uint64_t)(i))>>0)&0x00000000000000ffull); \
	}

#endif /* CSWAPH */ 
