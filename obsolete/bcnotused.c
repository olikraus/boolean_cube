/*

  bcnotused.c 
  
  boolean cube functions, which had been developed but are obsolete
  provided here only because to keep the outdated functions somewhere

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

*/


#include "bc.h"
#include <assert.h>
#include <stdio.h>


/*============================================================*/
/* block of 8 bit functions, which had been replaced by 16 bit functions */

/*
  For the sum calculation use signed 8 bit integer, because SSE2 only has a signed cmplt for 8 bit
  As a consequence, we are somehow loosing one bit in the sum, because saturation will be at 0x7f
  
  This function has been replaced by a 16 bit version
  
*/
#define _mm_adds(a,b) _mm_adds_epi8((a),(b))

void bcp_CalcBCLBinateSplitVariableTable8(bcp p, bcl l)
{
	int i, blk_cnt = p->blk_cnt;
	int j, list_cnt = l->cnt;
	
	bc zero_cnt_cube[4];
	bc one_cnt_cube[4];
	

	__m128i c;  // current block from the current cube from the list
	__m128i t;	// temp block
	__m128i oc0, oc1, oc2, oc3;	// one count
	__m128i zc0, zc1, zc2, zc3;	// zero count
	__m128i mc; // mask cube for the lowest bit in each byte

	/* constuct the byte mask: we need the lowest bit of each byte */
	mc = _mm_setzero_si128();
	mc = _mm_insert_epi16(mc, 0x0101, 0);
	mc = _mm_insert_epi16(mc, 0x0101, 1);
	mc = _mm_insert_epi16(mc, 0x0101, 2);
	mc = _mm_insert_epi16(mc, 0x0101, 3);
	mc = _mm_insert_epi16(mc, 0x0101, 4);
	mc = _mm_insert_epi16(mc, 0x0101, 5);
	mc = _mm_insert_epi16(mc, 0x0101, 6);
	mc = _mm_insert_epi16(mc, 0x0101, 7);

	/* "misuse" the cubes as SIMD storage area for the counters */
	zero_cnt_cube[0] = bcp_GetGlobalCube(p, 4);
	zero_cnt_cube[1] = bcp_GetGlobalCube(p, 5);
	zero_cnt_cube[2] = bcp_GetGlobalCube(p, 6);
	zero_cnt_cube[3] = bcp_GetGlobalCube(p, 7);
	
	one_cnt_cube[0] = bcp_GetGlobalCube(p, 8);
	one_cnt_cube[1] = bcp_GetGlobalCube(p, 9);
	one_cnt_cube[2] = bcp_GetGlobalCube(p, 10);
	one_cnt_cube[3] = bcp_GetGlobalCube(p, 11);
	
	/* loop over the blocks */
	for( i = 0; i < blk_cnt; i++ )
	{
		/* clear all the conters for the current block */
		zc0 = _mm_setzero_si128();
		zc1 = _mm_setzero_si128();
		zc2 = _mm_setzero_si128();
		zc3 = _mm_setzero_si128();
		oc0 = _mm_setzero_si128();
		oc1 = _mm_setzero_si128();
		oc2 = _mm_setzero_si128();
		oc3 = _mm_setzero_si128();
		
		for( j = 0; j < list_cnt; j++ )
		{
                  if ( l->flags[j] == 0 )
                  {
			/*
				Goal: 
					Count, how often 01 (zero) and 10 (one) do appear in the list at a specific cube position
					This means, for a cube with x variables, we will generate 2*x numbers
					For this count we have to ignore 11 (don't care) values.
				Assumption:
					The code 00 (illegal) is assumed to be absent.
				Idea:
					Because only code 11, 01 and 10 will be there in the cubes, it will be good enough
					to count the 0 bits from 01 and 10.
					Each counter will be 8 bit only (hoping that this is enough) with satuartion at 255 (thanks to SIMD instructions)
					In order to add or not add the above "0" (from the 01 or 10 code) to the counter, we will just mask and invert the 0 bit.
				
				Example:
					Assume a one variable at bit pos 0/1:
						xxxxxx10		code for value "one" at bits 0/1
					this is inverted and masked with one SIMD instruction:
						00000001		increment value for the counter
					this value is then added (with sturation) to the "one" counter 
					if, on the other hand, there would be a don't care or zero, it would look like this:
						xxxxxx01		code for value "zero" at bits 0/1
						xxxxxx11		code for value "don't care" at bits 0/1
					the invert and mask operation for bit 0 will generate a 0 byte in both cases:
						00000000		increment value for the counter in case of "zero" and "don't care" value
			*/
			c = _mm_loadu_si128(bcp_GetBCLCube(p, l, j)+i);
			/* handle variable at bits 0/1 */
			t = _mm_andnot_si128(c, mc);		// flip the lowerst bit and mask the lowerst bit in each byte: the "10" code for value "one" will become "00000001"
			oc0 = _mm_adds(oc0, t);		// sum the "one" value with saturation
			c = _mm_srai_epi16(c,1);			// shift right to proceed with the "zero" value
			t = _mm_andnot_si128(c, mc);
			zc0 = _mm_adds(zc0, t);
			
			c = _mm_srai_epi16(c,1);			// shift right to process the next variable

			/* handle variable at bits 2/3 */
			t = _mm_andnot_si128(c, mc);
			oc1 = _mm_adds(oc1, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc1 = _mm_adds(zc1, t);

			c = _mm_srai_epi16(c,1);

			/* handle variable at bits 4/5 */
			t = _mm_andnot_si128(c, mc);
			oc2 = _mm_adds(oc2, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc2 = _mm_adds(zc2, t);

			c = _mm_srai_epi16(c,1);

			/* handle variable at bits 6/7 */
			t = _mm_andnot_si128(c, mc);
			oc3 = _mm_adds(oc3, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc3 = _mm_adds(zc3, t);
		}
		
              } // for j list
              /* store the registers for counting zeros and ones */
              _mm_storeu_si128(zero_cnt_cube[0] + i, zc0);
              _mm_storeu_si128(zero_cnt_cube[1] + i, zc1);
              _mm_storeu_si128(zero_cnt_cube[2] + i, zc2);
              _mm_storeu_si128(zero_cnt_cube[3] + i, zc3);
              
              _mm_storeu_si128(one_cnt_cube[0] + i, oc0);
              _mm_storeu_si128(one_cnt_cube[1] + i, oc1);
              _mm_storeu_si128(one_cnt_cube[2] + i, oc2);
              _mm_storeu_si128(one_cnt_cube[3] + i, oc3);
	} // for i block
	
        /* 
          based on the calculated number of "one" and "zero" values, find a variable which fits best for splitting.
          According to R. Rudell in "Multiple-Valued Logic Minimization for PLA Synthesis" this should be the
          variable with the highest value of one_cnt + zero_cnt
        */
        
}


/*
  Precondition: call to 
    void bcp_CalcBCLBinateSplitVariableTable(bcp p, bcl l)  

  returns the binate variable for which the minimum of one's and zero's is max
*/
int bcp_GetBCLBalancedBinateSplitVariable8(bcp p, bcl l)
{
  int max_min_cnt = -1;
  int max_min_var = -1;
  
  int cube_idx;
  int blk_idx;
  int byte_idx;
  int one_cnt;
  int zero_cnt;
  int min_cnt;
  
  int i;
  
  //int j, oc, zc;
  
  bc zero_cnt_cube[4];
  bc one_cnt_cube[4];

  /* "misuse" the cubes as SIMD storage area for the counters */
  zero_cnt_cube[0] = bcp_GetGlobalCube(p, 4);
  zero_cnt_cube[1] = bcp_GetGlobalCube(p, 5);
  zero_cnt_cube[2] = bcp_GetGlobalCube(p, 6);
  zero_cnt_cube[3] = bcp_GetGlobalCube(p, 7);
  
  one_cnt_cube[0] = bcp_GetGlobalCube(p, 8);
  one_cnt_cube[1] = bcp_GetGlobalCube(p, 9);
  one_cnt_cube[2] = bcp_GetGlobalCube(p, 10);
  one_cnt_cube[3] = bcp_GetGlobalCube(p, 11);
  
  for( i = 0; i < p->var_cnt; i++ )
  {
          cube_idx = i & 3;
          blk_idx = i / 64;
          byte_idx = (i & 63)>>2;
          one_cnt = ((uint8_t *)(one_cnt_cube[cube_idx] + blk_idx))[byte_idx];
          zero_cnt = ((uint8_t *)(zero_cnt_cube[cube_idx] + blk_idx))[byte_idx];
    
          /*
          oc = 0;
          zc = 0;
          for( j = 0; j < l->cnt; j++ )
          {
            int value = bcp_GetCubeVar(p, bcp_GetBCLCube(p, l, j), i);
            if ( value == 1 ) zc++;
            else if ( value==2) oc++;
          }
          assert( one_cnt == oc );
          assert( zero_cnt == zc );
          */
          
          min_cnt = one_cnt > zero_cnt ? zero_cnt : one_cnt;
          
          // printf("%d: one_cnt=%u zero_cnt=%u\n", i, one_cnt, zero_cnt);
          /* if min_cnt is zero, then there are only "ones" and "don't cares" or "zero" and "don't case". This is usually called unate in that variable */
    
          if ( min_cnt > 0 )
          {
            if ( max_min_cnt < min_cnt )
            {
                    max_min_cnt = min_cnt;
                    max_min_var = i;
            }
          }
  }
  
  /* max_min_var is < 0, then the complete BCL is unate in all variables */
  
  //printf("best variable for split: %d\n", max_min_var);
  return max_min_var;
}


/*
  Precondition: call to 
    void bcp_CalcBCLBinateSplitVariableTable(bcp p, bcl l)  

  returns the binate variable for which the number of one's plus number of zero's is max under the condition, that both number of once's and zero's are >0 

  Implementation without SSE2
*/

int bcp_GetBCLMaxBinateSplitVariableSimple8(bcp p, bcl l)
{
  int max_sum_cnt = -1;
  int max_sum_var = -1;
  int cube_idx;
  int blk_idx;
  int byte_idx;
  int one_cnt;
  int zero_cnt;
  
  int i;
  
  bc zero_cnt_cube[4];
  bc one_cnt_cube[4];
  
  if ( l->cnt == 0 )
    return -1;

  /* "misuse" the cubes as SIMD storage area for the counters */
  zero_cnt_cube[0] = bcp_GetGlobalCube(p, 4);
  zero_cnt_cube[1] = bcp_GetGlobalCube(p, 5);
  zero_cnt_cube[2] = bcp_GetGlobalCube(p, 6);
  zero_cnt_cube[3] = bcp_GetGlobalCube(p, 7);
  
  one_cnt_cube[0] = bcp_GetGlobalCube(p, 8);
  one_cnt_cube[1] = bcp_GetGlobalCube(p, 9);
  one_cnt_cube[2] = bcp_GetGlobalCube(p, 10);
  one_cnt_cube[3] = bcp_GetGlobalCube(p, 11);

  
  max_sum_cnt = -1;
  max_sum_var = -1;
  for( i = 0; i < p->var_cnt; i++ )
  {
          cube_idx = i & 3;
          blk_idx = i / 64;
          byte_idx = (i & 63)>>2;
          one_cnt = ((uint8_t *)(one_cnt_cube[cube_idx] + blk_idx))[byte_idx];
          zero_cnt = ((uint8_t *)(zero_cnt_cube[cube_idx] + blk_idx))[byte_idx];
          printf("%02x/%02x ", one_cnt, zero_cnt);
    
          if ( one_cnt > 0 && zero_cnt > 0 )
          {
            if ( max_sum_cnt < (one_cnt + zero_cnt) )
            {
                    max_sum_cnt = one_cnt + zero_cnt;
                    max_sum_var = i;
            }
          }
  }  
  printf("\n");
  return max_sum_var;
}

/*
  Precondition: call to 
    void bcp_CalcBCLBinateSplitVariableTable(bcp p, bcl l)  

  returns the binate variable for which the number of one's plus number of zero's is max under the condition, that both number of once's and zero's are >0 

  SSE2 Implementation
*/
int bcp_GetBCLMaxBinateSplitVariable8(bcp p, bcl l)
{
  int max_sum_cnt = -1;
  int max_sum_var = -1;
  int i, b;
  __m128i c;
  __m128i z;
  __m128i o;
  
  __m128i c_cmp = _mm_setzero_si128();
  __m128i c_max = _mm_setzero_si128();
  __m128i c_idx = _mm_setzero_si128();
  __m128i c_max_idx = _mm_setzero_si128();
  
  uint8_t m_base_idx[16] = { 0, 4, 8, 12,  16, 20, 24, 28,  32, 36, 40, 44,  48, 52, 56, 60 };
  uint8_t m_base_inc[16] = { 1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1 };
  
  uint8_t m_idx[16];
  uint8_t m_max[16];
  
  bc zero_cnt_cube[4];
  bc one_cnt_cube[4];
  
  if ( l->cnt == 0 )
    return -1;

  /* "misuse" the cubes as SIMD storage area for the counters */
  zero_cnt_cube[0] = bcp_GetGlobalCube(p, 4);
  zero_cnt_cube[1] = bcp_GetGlobalCube(p, 5);
  zero_cnt_cube[2] = bcp_GetGlobalCube(p, 6);
  zero_cnt_cube[3] = bcp_GetGlobalCube(p, 7);
  
  one_cnt_cube[0] = bcp_GetGlobalCube(p, 8);
  one_cnt_cube[1] = bcp_GetGlobalCube(p, 9);
  one_cnt_cube[2] = bcp_GetGlobalCube(p, 10);
  one_cnt_cube[3] = bcp_GetGlobalCube(p, 11);

  for( b = 0; b < p->blk_cnt; b++ )
  {
      c_idx = _mm_loadu_si128((__m128i *)m_base_idx);
      c_max = _mm_setzero_si128();
      c_max_idx = _mm_setzero_si128();
    
      for( i = 0; i < 4; i++ )
      {
        z = _mm_loadu_si128(zero_cnt_cube[i]+b);
        o = _mm_loadu_si128(one_cnt_cube[i]+b);

        /*
        printf("a%d: ", i);
        print128_num(o);
        
        printf("a%d: ", i);
        print128_num(z);
        */
        
        // idea: if either count is zero, then set also the other count to zero
        // because we don't want any index on unate variables, we clear both counts if one count is zero
        c = _mm_cmpeq_epi8(z, _mm_setzero_si128());     // check for zero count of zeros
        o = _mm_andnot_si128 (c, o);                                    // and clear the one count if the zero count is zero
        c = _mm_cmpeq_epi8(o, _mm_setzero_si128());     // check for zero count of ones
        z = _mm_andnot_si128 (c, z);                                    // and clear the zero count if the one count is zero

        /*
        printf("b     %d: ", i);
        print128_num(o);
        
        printf("b     %d: ", i);
        print128_num(z);
        */
        
        // at this point either both o and z are zero or both are not zero        
        // now, calculate the sum of both counts and store the sum in z, o is not required any more
        z = _mm_adds(z, o);

        /*
        printf("z     %d: ", i);
        print128_num(z);
        */
        
        c_cmp = _mm_cmplt_epi8( c_max, z );

        c_max = _mm_or_si128( _mm_andnot_si128(c_cmp, c_max), _mm_and_si128(c_cmp, z) );                        // update max value if required
        c_max_idx = _mm_or_si128( _mm_andnot_si128(c_cmp, c_max_idx), _mm_and_si128(c_cmp, c_idx) );    // update index value if required        
        
        c_idx = _mm_adds_epu8(c_idx, _mm_loadu_si128((__m128i *)m_base_inc));   // we just add 1 to the value, so the highest value per byte is 64
      }
      
      _mm_storeu_si128( (__m128i *)m_max, c_max );
      _mm_storeu_si128( (__m128i *)m_idx, c_max_idx );
      for( i = 0; i < 16; i++ )
        if ( m_max[i] > 0 )
          if ( max_sum_cnt < m_max[i] )
          {
            max_sum_cnt = m_max[i];
            max_sum_var = m_idx[i] + b*p->vars_per_blk_cnt;
          }
  }
  
  /*
  {
    int mv = bcp_GetBCLMaxBinateSplitVariableSimple(p, l);
    if ( max_sum_var != mv )
    {
      printf("failed max_sum_var=%d, mv=%d\n", max_sum_var, mv);
      //bcp_ShowBCL(p, l);
    }
  }
  */
  
  return max_sum_var;
}


/*
  Precondition: call to 
    void bcp_CalcBCLBinateSplitVariableTable(bcp p, bcl l)  

  return 0 if there is any variable which has one's and "zero's in the table
  otherwise this function returns 1
*/
int bcp_IsBCLUnate8(bcp p)
{
  int b;
  bc zero_cnt_cube[4];
  bc one_cnt_cube[4];
  __m128i z;
  __m128i o;

  /* "misuse" the cubes as SIMD storage area for the counters */
  zero_cnt_cube[0] = bcp_GetGlobalCube(p, 4);
  zero_cnt_cube[1] = bcp_GetGlobalCube(p, 5);
  zero_cnt_cube[2] = bcp_GetGlobalCube(p, 6);
  zero_cnt_cube[3] = bcp_GetGlobalCube(p, 7);
  
  one_cnt_cube[0] = bcp_GetGlobalCube(p, 8);
  one_cnt_cube[1] = bcp_GetGlobalCube(p, 9);
  one_cnt_cube[2] = bcp_GetGlobalCube(p, 10);
  one_cnt_cube[3] = bcp_GetGlobalCube(p, 11);

  for( b = 0; b < p->blk_cnt; b++ )
  {
        z = _mm_cmpeq_epi8(_mm_loadu_si128(zero_cnt_cube[0]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
        o = _mm_cmpeq_epi8(_mm_loadu_si128(one_cnt_cube[0]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
                       // if ones and zeros are present, then both values are 0x00 and the reseult of the or is also 0x00
                      // this means, if there is any bit in c set to 0, then BCL is not unate and we can finish this procedure
        if ( _mm_movemask_epi8(_mm_or_si128(o, z)) != 0x0ffff )
          return 0;
        
        z = _mm_cmpeq_epi8(_mm_loadu_si128(zero_cnt_cube[1]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
        o = _mm_cmpeq_epi8(_mm_loadu_si128(one_cnt_cube[1]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
                       // if ones and zeros are present, then both values are 0x00 and the reseult of the or is also 0x00
                      // this means, if there is any bit in c set to 0, then BCL is not unate and we can finish this procedure
        if ( _mm_movemask_epi8(_mm_or_si128(o, z)) != 0x0ffff )
          return 0;

        z = _mm_cmpeq_epi8(_mm_loadu_si128(zero_cnt_cube[2]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
        o = _mm_cmpeq_epi8(_mm_loadu_si128(one_cnt_cube[2]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
                       // if ones and zeros are present, then both values are 0x00 and the reseult of the or is also 0x00
                      // this means, if there is any bit in c set to 0, then BCL is not unate and we can finish this procedure
        if ( _mm_movemask_epi8(_mm_or_si128(o, z)) != 0x0ffff )
          return 0;

        z = _mm_cmpeq_epi8(_mm_loadu_si128(zero_cnt_cube[3]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
        o = _mm_cmpeq_epi8(_mm_loadu_si128(one_cnt_cube[3]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
                       // if ones and zeros are present, then both values are 0x00 and the reseult of the or is also 0x00
                      // this means, if there is any bit in c set to 0, then BCL is not unate and we can finish this procedure
        if ( _mm_movemask_epi8(_mm_or_si128(o, z)) != 0x0ffff )
          return 0;
  }
  return 1;
}


/*============================================================*/
/* functions, which are in general obsolete, but might be reused at some point in time */


/*
  this differs from 
    bcp_GetBCLMaxBinateSplitVariable(bcp p, bcl l)
  by also considering unate variables

NOT TESTED
*/

int bcp_GetBCLMaxSplitVariable(bcp p, bcl l)
{
  int max_sum_cnt = -1;
  int max_sum_var = -1;
  int i, b;
  __m128i z;
  __m128i o;
  
  __m128i c_cmp = _mm_setzero_si128();
  __m128i c_max = _mm_setzero_si128();
  __m128i c_idx = _mm_setzero_si128();
  __m128i c_max_idx = _mm_setzero_si128();
  
  uint16_t m_base_idx[8] = { 0, 8, 16, 24,  32, 40, 48, 56 };
  uint16_t m_base_inc[8] = { 1, 1, 1, 1,   1, 1, 1, 1 };
  
  uint16_t m_idx[8];
  uint16_t m_max[8];
  
  bc zero_cnt_cube[8];
  bc one_cnt_cube[8];

  if ( l->cnt == 0 )
    return -1;
  
  
  /* "misuse" the cubes as SIMD storage area for the counters */
  zero_cnt_cube[0] = bcp_GetGlobalCube(p, 4);
  zero_cnt_cube[1] = bcp_GetGlobalCube(p, 5);
  zero_cnt_cube[2] = bcp_GetGlobalCube(p, 6);
  zero_cnt_cube[3] = bcp_GetGlobalCube(p, 7);
  zero_cnt_cube[4] = bcp_GetGlobalCube(p, 8);
  zero_cnt_cube[5] = bcp_GetGlobalCube(p, 9);
  zero_cnt_cube[6] = bcp_GetGlobalCube(p, 10);
  zero_cnt_cube[7] = bcp_GetGlobalCube(p, 11);
  
  one_cnt_cube[0] = bcp_GetGlobalCube(p, 12);
  one_cnt_cube[1] = bcp_GetGlobalCube(p, 13);
  one_cnt_cube[2] = bcp_GetGlobalCube(p, 14);
  one_cnt_cube[3] = bcp_GetGlobalCube(p, 15);
  one_cnt_cube[4] = bcp_GetGlobalCube(p, 16);
  one_cnt_cube[5] = bcp_GetGlobalCube(p, 17);
  one_cnt_cube[6] = bcp_GetGlobalCube(p, 18);
  one_cnt_cube[7] = bcp_GetGlobalCube(p, 19);

  for( b = 0; b < p->blk_cnt; b++ )
  {
      c_idx = _mm_loadu_si128((__m128i *)m_base_idx);
      c_max = _mm_setzero_si128();
      c_max_idx = _mm_setzero_si128();
    
      for( i = 0; i < 8; i++ )
      {
        z = _mm_loadu_si128(zero_cnt_cube[i]+b);
        o = _mm_loadu_si128(one_cnt_cube[i]+b);

        // calculate the sum of both counts and store the sum in z, o is not required any more
        z = _mm_adds_epi16(z, o);

        c_cmp = _mm_cmplt_epi16( c_max, z );

        c_max = _mm_or_si128( _mm_andnot_si128(c_cmp, c_max), _mm_and_si128(c_cmp, z) );                        // update max value if required
        c_max_idx = _mm_or_si128( _mm_andnot_si128(c_cmp, c_max_idx), _mm_and_si128(c_cmp, c_idx) );    // update index value if required        
        
        c_idx = _mm_adds_epu16(c_idx, _mm_loadu_si128((__m128i *)m_base_inc));   // we just add 1 to the value, so the highest value per byte is 64
      }
      
      _mm_storeu_si128( (__m128i *)m_max, c_max );
      _mm_storeu_si128( (__m128i *)m_idx, c_max_idx );
      for( i = 0; i < 8; i++ )
        if ( m_max[i] > 0 )
          if ( max_sum_cnt < m_max[i] )
          {
            max_sum_cnt = m_max[i];
            max_sum_var = m_idx[i] + b*p->vars_per_blk_cnt;
          }
  }
  return max_sum_var;
}



/*
  do an "and" between all elements of l
  the result of this operation is stored in cube "r"
  Interpretation:
    If "r" is illegal (contains one or more 00 codes), then the list has the BINATE property
    Varibales, which have the illegal code, can be used for the cofactor split operation
    If "r" is not illegal, then the list has the UNATE property
      A list with the UNATE property has the tautology property if there is the tautology block included
*/
void bcp_AndBCL(bcp p, bc r, bcl l)
{
  int i, blk_cnt = p->blk_cnt;
  int j, list_cnt = l->cnt;
  bc lc;
  __m128i m;
  bcp_CopyGlobalCube(p, r, 3);          // global cube 3 has all vars set to don't care (tautology block)
  

  for( i = 0; i < blk_cnt; i++ )
  {
    m = _mm_loadu_si128(r+i);
    for( j = 0; j < list_cnt; j++ )
    {
        if ( (l->flags[j]&1) == 0 )             // check delete flag
        {
          lc = bcp_GetBCLCube(p, l, j);
          m = _mm_and_si128(m, _mm_loadu_si128(lc+i));
        }
    }
    _mm_storeu_si128(r+i, m);
  }
}




