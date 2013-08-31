//Thanks to Todd Markley for putting this together, based on GNU's <byteswap.h>

#ifndef CSWAPH
#define CSWAPH 1

#define cswap_16(x) (((((uint16_t)(((uint8_t*)(x))[1]))<<0)&0x00ff) + \
                     ((((uint16_t)(((uint8_t*)(x))[0]))<<8)&0xff00))

#define cswap_24(x) (((((uint32_t)(((uint8_t*)(x))[2]))<<0)&0x0000ff) + \
                     ((((uint32_t)(((uint8_t*)(x))[1]))<<8)&0x00ff00) + \
                     ((((uint32_t)(((uint8_t*)(x))[0]))<<16)&0xff0000))

#define cswap_32(x) (((((uint32_t)(((uint8_t*)(x))[3]))<<0)&0x000000ff) + \
                    ((((uint32_t)(((uint8_t*)(x))[2]))<<8)&0x0000ff00) + \
                    ((((uint32_t)(((uint8_t*)(x))[1]))<<16)&0x00ff0000) + \
                    ((((uint32_t)(((uint8_t*)(x))[0]))<<24)&0xff000000))

#define cswap_64(x) \
      (((((uint64_t)(((uint8_t*)(x))[7]))<<0)&0x00000000000000ffull) + \
      ((((uint64_t)(((uint8_t*)(x))[6]))<<8)&0x000000000000ff00ull) + \
      ((((uint64_t)(((uint8_t*)(x))[5]))<<16)&0x0000000000ff0000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[4]))<<24)&0x00000000ff000000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[3]))<<32)&0x000000ff00000000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[2]))<<40)&0x0000ff0000000000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[1]))<<48)&0x00ff000000000000ull) + \
      ((((uint64_t)(((uint8_t*)(x))[0]))<<56)&0xff00000000000000ull))


#endif /* CSWAPH */ 
