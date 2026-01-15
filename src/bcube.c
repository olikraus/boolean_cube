/*

  bcube.c
  
  boolean cube core functions

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/


  __m128i _mm_srai_epi16 (__m128i a, int imm8)
    Shift a to the right by imm8 bits. For bcc the size (epi16) doesn't matter.
    
  int _mm_movemask_epi8 (__m128i a)
    Create mask from the most significant bit of each 8-bit element in a and return the result.
    There are 16 bytes in _m128i, so the result will contain 16 bits (a value between 0x0000 and 0x0ffff)
    This command only makes sense together with _mm_cmpeq
    
  __m128i _mm_cmpeq_epi16 (__m128i a, __m128i b)
  __m128i _mm_cmpeq_epi8 (__m128i a, __m128i b)
    In bcc we have only two bit elements, so again whether to use epi16 or epi8 doesn't matter
    Compare packed 16/8-bit integers in a and b for equality, and return the result at the same position as 0x0ffff or 0x00000.
    Pattern: _mm_movemask_epi8(_mm_cmpeq_epi8(a,b)) returns 0xffff if a and b are equal
*/

#include "bc.h"
#include <string.h>
#include <assert.h>


#define m128i_is_equal(m1, m2) \
  ((_mm_movemask_epi8(_mm_cmpeq_epi16((m1),(m2))) == 0xFFFF)?1:0)


void bcp_ClrCube(bcp p, bc c)
{
  memset(c, 0xff, p->bytes_per_cube_cnt);       // assign don't care
}

void bcp_CopyCube(bcp p, bc dest, bc src)
{
  memcpy(dest, src, p->bytes_per_cube_cnt);
}

int bcp_CompareCube(bcp p, bc a, bc b)
{
  return memcmp((void *)a, (void *)b, p->bytes_per_cube_cnt);
}



void bcp_SetCubeVar(bcp p, bc c, unsigned var_pos, unsigned value)
{  
  unsigned idx = var_pos/8;
  uint16_t *ptr = (uint16_t *)c;
  uint16_t mask = ~(3 << ((var_pos&7)*2));
  assert( var_pos < p->var_cnt );
  ptr[idx] &= mask;
  ptr[idx] |= value  << ((var_pos&7)*2);
}

/*
unsigned bcp_GetCubeVar(bcp p, bc c, unsigned var_pos)
{
  return (((uint16_t *)c)[var_pos/8] >> ((var_pos&7)*2)) & 3;
}
#define bcp_GetCubeVar(p, c, var_pos) \
  ((((uint16_t *)(c))[(var_pos)/8] >> (((var_pos)&7)*2)) & 3)
*/

const char *bcp_GetStringFromCube(bcp p, bc c)
{
  int i, var_cnt = p->var_cnt;
  char *s = p->cube_to_str; 
  for( i = 0; i < var_cnt; i++ )
  {
    s[i] = "x01-"[bcp_GetCubeVar(p, c, i)];
  }
  s[var_cnt] = '\0';
  return s;
}



/*
  use string "s" to fill the content of cube "c"
    '0' --> bit value 01
    '1' --> bit value 10
    '-' --> bit value 11
    'x' (or any other char > 32 --> bit value "00" (which is the illegal value)
    ' ', '\t' --> ignored
    '\0', '\r', '\n' --> reading from "s" will stop
  
*/
void bcp_SetCubeByStringPointer(bcp p, bc c,  const char **s)
{
  int i, var_cnt = p->var_cnt;
  unsigned v;
  for( i = 0; i < var_cnt; i++ )
  {
    while( **s == ' ' || **s == '\t' )            // skip white space
      (*s)++;
    if ( **s == '0' ) { v = 1; }
    else if ( **s == '1' ) { v = 2; }
    else if ( **s == '-' ) { v = 3; }
    else if ( **s == 'x' ) { v = 0; }
    else { v = 3; }
    if ( **s != '\0' && **s != '\r' && **s != '\n' )       // stop looking at further chars if the line/string ends
      (*s)++;
    bcp_SetCubeVar(p, c, i, v);
  }
}

void bcp_SetCubeByString(bcp p, bc c, const char *s)
{
  bcp_SetCubeByStringPointer(p, c, &s);
}



int bcp_IsTautologyCube(bcp p, bc c)
{
  // assumption: Unused vars are set to 3 (don't care)
  int i, cnt = p->blk_cnt;
  __m128i t = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 3));
  
  for( i = 0; i < cnt; i++ )
    if ( m128i_is_equal(_mm_loadu_si128(c+i), t) == 0 )
      return 0;
  return 1;
}


/*
  calculate intersection of a and b, result is stored in r
  return 0, if there is no intersection
*/
int bcp_IntersectionCube(bcp p, bc r, bc a, bc b)
{
  int i, cnt = p->blk_cnt;
  __m128i z = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 1));
  __m128i rr;
  uint16_t f = 0x0ffff;
  for( i = 0; i < cnt; i++ )
  {    
    rr = _mm_and_si128(_mm_loadu_si128(a+i), _mm_loadu_si128(b+i));      // calculate the intersection
    _mm_storeu_si128(r+i, rr);          // and store the intersection in the destination cube
    /*
      each value has the bits illegal:00, zero:01, one:10, don't care:11
      goal is to find, if there are any illegal variables bit pattern 00.
      this is done with the following steps:
        1. OR operation: r|r>>1 --> the lower bit will be 0 only, if we have the illegal var
        2. AND operation with the zero mask --> so there will be only 00 (illegal) or 01 (not illegal)
        3. CMP operation with the zero mask, the data size (epi16) does not matter here
        4. movemask operation to see, whether the result is identical to the zero mask (if so, then the intersection exists)
    
        Short form: 00? --> x0 --> 00 == 00?
    */    
    f &= _mm_movemask_epi8(_mm_cmpeq_epi16(_mm_and_si128(_mm_or_si128( rr, _mm_srai_epi16(rr,1)), z), z));
  }
  if ( f == 0xffff )
    return 1;
  return 0;
}

/*
  Each variable value will be mapped in the following way:
    00 --> 01 (can be ignored)
    01 --> 01
    10 --> 01
    11 --> 00

    This means, that we will have the code 01 for any dc
*/
void bcp_GetVariableMask(bcp p, bc mask, bc c)
{
  int i, cnt = p->blk_cnt;
  __m128i z = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 1));       // idx 0: all illegal (00), idx 1: all zero (01), idx 2: all one (10) and idx 3: all don't care (11)
  __m128i r;
  for( i = 0; i < cnt; i++ )
  {    
    r = _mm_loadu_si128(c+i); 
    r = _mm_and_si128( r, _mm_srai_epi16(r,1));          // r = r & (r >> 1)     this will generate x1 for DC values
    r = _mm_andnot_si128(r, z);                                    // r = ~r & 01   this will generate 00 for DC and 01 for "10" and "01" (and also for "00")
    _mm_storeu_si128(mask+i, r);          // and store the result in the destination cube    
  }
}

/*
	00 --> 11
	01 --> 10
	10 --> 01
	11 --> 11

	3 Jun 24: NOT TESTED
*/
void bcp_InvertCube(bcp p, bc c)
{
  int i, cnt = p->blk_cnt;
  __m128i z = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 1));
  __m128i r;
  for( i = 0; i < cnt; i++ )
  {    
    r = _mm_loadu_si128(c+i); 
    r = _mm_and_si128( r, _mm_srai_epi16(r,1));          // r = r & (r >> 1)     this will generate x1 for DC values
    r = _mm_andnot_si128(r, z);                                    // r = ~r & 01   this will generate 00 for DC and 01 for "10" and "01" (and also for "00")
    r = _mm_or_si128(r, _mm_slli_epi16(r,1));	  		// r = r | (r <<1)  00 for DC and 11 for all other values
    r = _mm_xor_si128( r, _mm_loadu_si128(c+i) );	// invert the the original values, except for DC
    _mm_storeu_si128(c+i, r);          // and store the result in the cube 
  }
}

/*
  does a bitwise and operation and checks whether the result is bitwise zeor
  This is used together with bcp_GetVariableMask()
*/
int bcp_IsAndZero(bcp p, bc a, bc b)
{
  int i, cnt = p->blk_cnt;
  __m128i zz = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 0));  // idx 0: all illegal (00), idx 1: all zero (01), idx 2: all one (10) and idx 3: all don't care (11)
  __m128i rr;
  
  for( i = 0; i < cnt; i++ )
  {    
    rr = _mm_and_si128(_mm_loadu_si128(a+i), _mm_loadu_si128(b+i));      // calculate bitwise AND
    if ( _mm_movemask_epi8(_mm_cmpeq_epi8(rr, zz)) != 0xFFFF )          // check if any bit is not zero
      return 0;         // some none-zero bits detected
  }
  return 1;     // all zero after bitwise AND
}

/*
  do a bitwise or and return the number of bits in the result;
*/
unsigned bcp_OrBitCnt(bcp p, bc r, bc a, bc b)
{
  int i, cnt = p->blk_cnt;
  __m128i rr;
  unsigned bitcnt = 0;
  for( i = 0; i < cnt; i++ )
  {    
    rr = _mm_or_si128(_mm_loadu_si128(a+i), _mm_loadu_si128(b+i));      // calculate bitwise OR
    _mm_storeu_si128(r+i, rr);          // and store the bitwise or result in the destination cube
    
    bitcnt += __builtin_popcountll(_mm_cvtsi128_si64(_mm_unpackhi_epi64(rr, rr)));
    bitcnt += __builtin_popcountll(_mm_cvtsi128_si64(rr));    
  }
  return bitcnt;
}

int bcp_IsIntersectionCube(bcp p, bc a, bc b)
{
  int i, cnt = p->blk_cnt;
  __m128i z = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 1));  // idx 0: all illegal (00), idx 1: all zero (01), idx 2: all one (10) and idx 3: all don't care (11)
  __m128i rr;
  uint16_t f = 0x0ffff;
  for( i = 0; i < cnt; i++ )
  {    
    rr = _mm_and_si128(_mm_loadu_si128(a+i), _mm_loadu_si128(b+i));      // calculate the intersection
    /*
      each value has the bits illegal:00, zero:01, one:10, don't care:11
      goal is to find, if there are any illegal variables bit pattern 00.
      this is done with the following steps:
        1. OR operation: r|r>>1 --> the lower bit will be 0 only, if we have the illegal var
        2. AND operation with the zero mask --> so there will be only 00 (illegal) or 01 (not illegal)
        3. CMP operation with the zero mask, the data size (epi16) does not matter here
        4. movemask operation to see, whether the result is identical to the zero mask (if so, then the intersection exists)
    
        Short form: 00? --> x0 --> 00 == 00?
    */    
    f &= _mm_movemask_epi8(_mm_cmpeq_epi16(_mm_and_si128(_mm_or_si128( rr, _mm_srai_epi16(rr,1)), z), z));
    if ( f != 0xffff )
      return 0;
  }
  return 1;
}

/* returns 1, if cube "c" is illegal, return 0 if "c" is not illegal */
int bcp_IsIllegal(bcp p, bc c)
{
  int i, cnt = p->blk_cnt;
  __m128i z = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 1));
  __m128i cc;
  uint16_t f = 0x0ffff;
  for( i = 0; i < cnt; i++ )
  {
    cc = _mm_loadu_si128(c+i);      // load one block
    f &= _mm_movemask_epi8(_mm_cmpeq_epi16(_mm_and_si128(_mm_or_si128( cc, _mm_srai_epi16(cc,1)), z), z));
  }
  if ( f == 0xffff )
    return 0;
  return 1;
}

/*
  return the number of 01 or 10 values in a legal cube.

  Jan 2025: Somehow the description above doesn't fit to the code: The code will count 11 twice I assume...

  called by bcp_GetBCLVarCntList()
*/
int bcp_GetCubeVariableCount(bcp p, bc cube)
{
  int i, cnt = p->blk_cnt;
  int delta = 0;
    __m128i c;
  for( i = 0; i < cnt; i++ )
  {
    c = _mm_loadu_si128(cube+i);      // load one block from cube
    
    /* use gcc builtin command, also use -march=silvermont with gcc to generate the popcount assembler command */
    delta += __builtin_popcountll(~_mm_cvtsi128_si64(_mm_unpackhi_epi64(c, c)));
    delta += __builtin_popcountll(~_mm_cvtsi128_si64(c));
  }  
  return delta;
}

int bcp_GetCubeDelta(bcp p, bc a, bc b)
{
  int i, cnt = p->blk_cnt;
  int delta = 0;
  __m128i zeromask = _mm_loadu_si128(bcp_GetGlobalCube(p, 1));
  __m128i c;

  for( i = 0; i < cnt; i++ )
  {
    c = _mm_loadu_si128(a+i);      // load one block from a
    c = _mm_and_si128(c,  _mm_loadu_si128(b+i)); // "and" between a&b: how often will there be 00 (=illegal)?
    c = _mm_or_si128( c, _mm_srai_epi16(c,1));  // how often will be there x0?
    c = _mm_andnot_si128(c, zeromask);          // invert c (look for x1) and mask with the zero mask to get 01
    delta += __builtin_popcountll(_mm_cvtsi128_si64(_mm_unpackhi_epi64(c, c)));
    delta += __builtin_popcountll(_mm_cvtsi128_si64(c));
  }
  
  return delta;
}


/*
  test, whether "b" is a subset of "a"
  returns:      
    1: yes, "b" is a subset of "a"
    0: no, "b" is not a subset of "a"
*/
int bcp_IsSubsetCube(bcp p, bc a, bc b)
{
  int i;
  __m128i bb;
  for( i = 0; i < p->blk_cnt; i++ )
  {    
      /* a&b == b ?*/
    bb = _mm_loadu_si128(b+i);
    if ( _mm_movemask_epi8(_mm_cmpeq_epi16(_mm_and_si128(_mm_loadu_si128(a+i), bb), bb )) != 0x0ffff )
      return 0;
  }
  return 1;
}

