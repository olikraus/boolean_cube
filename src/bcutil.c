/*

  bcutil.c
  
  boolean cube utility functions

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

*/

#include "bc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


/*
static __m128i m128i_get_n_bit_mask(uint16_t val, unsigned bit_pos)
{
  uint16_t a[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  val <<= bit_pos & 0xf;
  a[7-(bit_pos>>4)] = val;
  return _mm_loadu_si128((__m128i *)a);
}
*/

/*
#define m128i_get_n_bit_mask(val, bit_pos) \
  _mm_slli_si128(_mm_set_epi32(0, 0, 0, (val)<<(bit_pos&0x1f)), (bit_pos)>>5)
*/

/* limitation: the bitpattern in val must not cross a 32 bit boundery, for example val=3 and  bit_pos=31 is not allowed */
/*
#define m128i_get_n_bit_mask(val, bit_pos) \
  _mm_set_epi32(  \
    0==((bit_pos)>>5)?(((uint32_t)(val))<<(bit_pos&0x1f)):0, \
    1==((bit_pos)>>5)?(((uint32_t)(val))<<(bit_pos&0x1f)):0, \
    2==((bit_pos)>>5)?(((uint32_t)(val))<<(bit_pos&0x1f)):0, \
    3==((bit_pos)>>5)?(((uint32_t)(val))<<(bit_pos&0x1f)):0 ) 
*/

/*
#define m128i_get_1_bit_mask(bit_pos) \
  m128i_get_n_bit_mask(1, bit_pos)
*/

/*
#define m128i_get_bit_mask(bit_pos) \
  _mm_slli_si128(_mm_set_epi32 (0, 0, 0, 1<<(bit_pos&0x1f)), (bit_pos)>>5)
*/

/*
#define m128i_not(m) \
  _mm_xor_si128(m, _mm_set1_epi16(0x0ffff))
*/

/*
#define m128i_is_zero(m) \
  ((_mm_movemask_epi8(_mm_cmpeq_epi16((m),_mm_setzero_si128())) == 0xFFFF)?1:0)
*/


/* val=0..3, bit_pos=0..126 (must not be odd) */
/*
#define m128i_set_2_bit(m, val, bit_pos) \
 _mm_or_si128( m128i_get_n_bit_mask((val), (bit_pos)), _mm_andnot_si128(m128i_get_n_bit_mask(3, (bit_pos)), (m)) )
*/



void print128_num(__m128i var)
{
    uint8_t val[16];
    memcpy(val, &var, sizeof(val));
    printf("m128i: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n", 
           val[0], val[1], 
           val[2], val[3], 
           val[4], val[5], 
           val[6], val[7],
           val[8], val[9], 
           val[10], val[11], 
           val[12], val[13], 
           val[14], val[15]
  );
}

