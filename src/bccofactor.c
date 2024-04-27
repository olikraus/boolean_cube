/*

  bccofactor.c
  
  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

  
  
  contains function function which 
    - require calls to "bcp_CalcBCLBinateSplitVariableTable"
    - calculate the cofactor
    - find the right variable to do a recursiv split with help of a cofactor

*/

#include "bc.h"
#include <assert.h>



/* 16 bit version */
void bcp_CalcBCLBinateSplitVariableTable(bcp p, bcl l)
{
	int i, blk_cnt = p->blk_cnt;
	int j, list_cnt = l->cnt;
	
	bc zero_cnt_cube[8];
	bc one_cnt_cube[8];
	

	__m128i c;  // current block from the current cube from the list
	__m128i t;	// temp block
	__m128i oc0, oc1, oc2, oc3, oc4, oc5, oc6, oc7;	// one count
	__m128i zc0, zc1, zc2, zc3, zc4, zc5, zc6, zc7;	// zero count
	__m128i mc; // mask cube for the lowest bit in each byte

	/* constuct the byte mask: we need the lowest bit of each byte */
	mc = _mm_setzero_si128();
	mc = _mm_insert_epi16(mc, 1, 0);
	mc = _mm_insert_epi16(mc, 1, 1);
	mc = _mm_insert_epi16(mc, 1, 2);
	mc = _mm_insert_epi16(mc, 1, 3);
	mc = _mm_insert_epi16(mc, 1, 4);
	mc = _mm_insert_epi16(mc, 1, 5);
	mc = _mm_insert_epi16(mc, 1, 6);
	mc = _mm_insert_epi16(mc, 1, 7);

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
	
	/* loop over the blocks */
	for( i = 0; i < blk_cnt; i++ )
	{
		/* clear all the conters for the current block */
		zc0 = _mm_setzero_si128();
		zc1 = _mm_setzero_si128();
		zc2 = _mm_setzero_si128();
		zc3 = _mm_setzero_si128();
		zc4 = _mm_setzero_si128();
		zc5 = _mm_setzero_si128();
		zc6 = _mm_setzero_si128();
		zc7 = _mm_setzero_si128();
		oc0 = _mm_setzero_si128();
		oc1 = _mm_setzero_si128();
		oc2 = _mm_setzero_si128();
		oc3 = _mm_setzero_si128();
		oc4 = _mm_setzero_si128();
		oc5 = _mm_setzero_si128();
		oc6 = _mm_setzero_si128();
		oc7 = _mm_setzero_si128();
		
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
			oc0 = _mm_adds_epi16(oc0, t);		// sum the "one" value with saturation
			c = _mm_srai_epi16(c,1);			// shift right to proceed with the "zero" value
			t = _mm_andnot_si128(c, mc);
			zc0 = _mm_adds_epi16(zc0, t);
			
			c = _mm_srai_epi16(c,1);			// shift right to process the next variable

			/* handle variable at bits 2/3 */
			t = _mm_andnot_si128(c, mc);
			oc1 = _mm_adds_epi16(oc1, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc1 = _mm_adds_epi16(zc1, t);

			c = _mm_srai_epi16(c,1);

			/* handle variable at bits 4/5 */
			t = _mm_andnot_si128(c, mc);
			oc2 = _mm_adds_epi16(oc2, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc2 = _mm_adds_epi16(zc2, t);

			c = _mm_srai_epi16(c,1);

			/* handle variable at bits 6/7 */
			t = _mm_andnot_si128(c, mc);
			oc3 = _mm_adds_epi16(oc3, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc3 = _mm_adds_epi16(zc3, t);
                        
			c = _mm_srai_epi16(c,1);

			/* handle variable at bits 8/9 */
			t = _mm_andnot_si128(c, mc);
			oc4 = _mm_adds_epi16(oc4, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc4 = _mm_adds_epi16(zc4, t);
                        
			c = _mm_srai_epi16(c,1);

			/* handle variable at bits 10/11 */
			t = _mm_andnot_si128(c, mc);
			oc5 = _mm_adds_epi16(oc5, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc5 = _mm_adds_epi16(zc5, t);
                        
			c = _mm_srai_epi16(c,1);

			/* handle variable at bits 12/13 */
			t = _mm_andnot_si128(c, mc);
			oc6 = _mm_adds_epi16(oc6, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc6 = _mm_adds_epi16(zc6, t);
                        
			c = _mm_srai_epi16(c,1);

			/* handle variable at bits 14/15 */
			t = _mm_andnot_si128(c, mc);
			oc7 = _mm_adds_epi16(oc7, t);
			c = _mm_srai_epi16(c,1);
			t = _mm_andnot_si128(c, mc);
			zc7 = _mm_adds_epi16(zc7, t);
                  }  // flag test
		
                } // j, list loop
		/* store the registers for counting zeros and ones */
		_mm_storeu_si128(zero_cnt_cube[0] + i, zc0);
		_mm_storeu_si128(zero_cnt_cube[1] + i, zc1);
		_mm_storeu_si128(zero_cnt_cube[2] + i, zc2);
		_mm_storeu_si128(zero_cnt_cube[3] + i, zc3);
		_mm_storeu_si128(zero_cnt_cube[4] + i, zc4);
		_mm_storeu_si128(zero_cnt_cube[5] + i, zc5);
		_mm_storeu_si128(zero_cnt_cube[6] + i, zc6);
		_mm_storeu_si128(zero_cnt_cube[7] + i, zc7);
		
		_mm_storeu_si128(one_cnt_cube[0] + i, oc0);
		_mm_storeu_si128(one_cnt_cube[1] + i, oc1);
		_mm_storeu_si128(one_cnt_cube[2] + i, oc2);
		_mm_storeu_si128(one_cnt_cube[3] + i, oc3);
		_mm_storeu_si128(one_cnt_cube[4] + i, oc4);
		_mm_storeu_si128(one_cnt_cube[5] + i, oc5);
		_mm_storeu_si128(one_cnt_cube[6] + i, oc6);
		_mm_storeu_si128(one_cnt_cube[7] + i, oc7);
	
        /* 
          based on the calculated number of "one" and "zero" values, find a variable which fits best for splitting.
          According to R. Rudell in "Multiple-Valued Logic Minimization for PLA Synthesis" this should be the
          variable with the highest value of one_cnt + zero_cnt
        */
      } // i, block loop
}

/*
  Precondition: call to 
    void bcp_CalcBCLBinateSplitVariableTable(bcp p, bcl l)  

  returns the binate variable for which the number of one's plus number of zero's is max under the condition, that both number of once's and zero's are >0 

  Implementation without SSE2
*/

/* 16 bit version */
int bcp_GetBCLMaxBinateSplitVariableSimple(bcp p, bcl l)
{
  int max_sum_cnt = -1;
  int max_sum_var = -1;
  int cube_idx;
  int blk_idx;
  int word_idx;
  int one_cnt;
  int zero_cnt;
  
  int i;
  
  bc zero_cnt_cube[8];
  bc one_cnt_cube[8];

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

  
  max_sum_cnt = -1;
  max_sum_var = -1;
  for( i = 0; i < p->var_cnt; i++ )
  {
          cube_idx = i & 7;
          blk_idx = i / 64;
          word_idx = (i & 63)>>3;
          one_cnt = ((uint16_t *)(one_cnt_cube[cube_idx] + blk_idx))[word_idx];
          zero_cnt = ((uint16_t *)(zero_cnt_cube[cube_idx] + blk_idx))[word_idx];
          //printf("%02x/%02x ", one_cnt, zero_cnt);
    
          if ( one_cnt > 0 && zero_cnt > 0 )
          {
            if ( max_sum_cnt < (one_cnt + zero_cnt) )
            {
                    max_sum_cnt = one_cnt + zero_cnt;
                    max_sum_var = i;
            }
          }
  }  
  //printf("\n");
  return max_sum_var;
}



/*
  Precondition: call to 
    void bcp_CalcBCLBinateSplitVariableTable(bcp p, bcl l)  

  returns the binate variable for which the number of one's plus number of zero's is max under the condition, that both number of once's and zero's are >0 

  SSE2 Implementation
*/
/* 16 bit version */
int bcp_GetBCLMaxBinateSplitVariable(bcp p, bcl l)
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

        /*
        printf("a%d: ", i);
        print128_num(o);
        
        printf("a%d: ", i);
        print128_num(z);
        */
        
        // idea: if either count is zero, then set also the other count to zero
        // because we don't want any index on unate variables, we clear both counts if one count is zero
        c = _mm_cmpeq_epi16(z, _mm_setzero_si128());     // check for zero count of zeros
        o = _mm_andnot_si128 (c, o);                                    // and clear the one count if the zero count is zero
        c = _mm_cmpeq_epi16(o, _mm_setzero_si128());     // check for zero count of ones
        z = _mm_andnot_si128 (c, z);                                    // and clear the zero count if the one count is zero

        /*
        printf("b     %d: ", i);
        print128_num(o);
        
        printf("b     %d: ", i);
        print128_num(z);
        */
        
        // at this point either both o and z are zero or both are not zero        
        // now, calculate the sum of both counts and store the sum in z, o is not required any more
        z = _mm_adds_epi16(z, o);

        /*
        printf("z     %d: ", i);
        print128_num(z);
        */
        
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
  with the cube at postion "pos" within "l", check whether there are any other cubes, which are a subset of the cobe at postion "pos"
  The cubes, which are marked as subset are not deleted. This should done by a later call to bcp_BCLPurge()
*/
static void bcp_DoBCLSubsetCubeMark(bcp p, bcl l, int pos)
{
  int j;
  int cnt = l->cnt;
  bc c = bcp_GetBCLCube(p, l, pos);
  for( j = 0; j < cnt; j++ )
  {
    if ( j != pos && l->flags[j] == 0  )
    {
      /*
        test, whether "b" is a subset of "a"
        returns:      
          1: yes, "b" is a subset of "a"
          0: no, "b" is not a subset of "a"
      */
      if ( bcp_IsSubsetCube(p, c, bcp_GetBCLCube(p, l, j)) != 0 )
      {
        l->flags[j] = 1;      // mark the j cube as covered (to be deleted)
      }            
    }
  }  
}


/*
  check whether a variable is only DC in all cubes
*/
int bcp_IsBCLVariableDC(bcp p, bcl l, unsigned var_pos)
{
  int i;
  int cnt = l->cnt;
  bc c;
  for( i = 0; i < cnt; i++ )
  {
    if ( l->flags[i] == 0 )
    {
      c = bcp_GetBCLCube(p, l, i);
      if ( bcp_GetCubeVar(p, c, var_pos) != 3 )
        return 0;
    }
  }
  return 1;
}

/*
  check whether a variable is unate with respect to "value".
  This means only DC or the value may appear in all cubes at the given var_pos;
*/
int bcp_IsBCLVariableUnate(bcp p, bcl l, unsigned var_pos, unsigned value)
{
  int i;
  int cnt = l->cnt;
  bc c;
  unsigned v;
  /*
  if ( var_pos >= p->var_cnt )
  {
    printf("var_pos=%d, var_cnt=%d\n", var_pos, p->var_cnt);
  }
  */
  assert( var_pos < p->var_cnt );
  
  for( i = 0; i < cnt; i++ )
  {
    if ( l->flags[i] == 0 )
    {
      c = bcp_GetBCLCube(p, l, i);
      assert( c != NULL );
      v = bcp_GetCubeVar(p, c, var_pos);
      if ( v != 3 && v != value  )
        return 0;
    }
  }
  return 1;
}



/*
  Calculate the cofactor of the given list "l" with respect to the variable at "var_pos" and the given "value".
  "value" must be either 1 (zero) or 2 (one)
  
  This function will update and modify the list "l"

*/
void bcp_DoBCLOneVariableCofactor(bcp p, bcl l, unsigned var_pos, unsigned value)
{
  int i;
  int cnt = l->cnt;
  unsigned v;
  bc c;
  
  assert(value == 1 || value == 2);
  
  //printf("bcp_DoBCLOneVariableCofactor pre var_pos=%d value=%d\n", var_pos, value);
  //bcp_ShowBCL(p, l);
  for( i = 0; i < cnt; i++ )
  {
    if ( l->flags[i] == 0 )
    {
      c = bcp_GetBCLCube(p, l, i);
      v = bcp_GetCubeVar(p, c, var_pos);
      if ( v != 3 )  // is the variable already don't care for the cofactor variable?
      {
        if ( (v | value) == 3 ) // if no, then check if the variable would become don't care
        {
          bcp_SetCubeVar(p, c, var_pos, 3);   // yes, variable will become don't care
          bcp_DoBCLSubsetCubeMark(p, l, i);
        } // check for "becomes don't care'
      } // check for not don't carebcp_DoBCLOneVariableCofactor
    }
  } // i loop
  bcp_PurgeBCL(p, l);  // cleanup for bcp_DoBCLSubsetCubeMark()
}

/*
  same as "bcp_DoBCLOneVariableCofactor()" but will not alter "l"
  instead a new list is returned, which must be freed with bcp_DeleteBCL()
*/
bcl bcp_NewBCLCofacterByVariable(bcp p, bcl l, unsigned var_pos, unsigned value)
{
  bcl n = bcp_NewBCLByBCL(p, l);
  if ( n == NULL )
    return NULL;
  bcp_DoBCLOneVariableCofactor(p, n, var_pos, value);
  return n;
}



/*
  Calculate the cofactor of l against c
  c might be part of l
  additionally ignore the element at position exclude (which is assumed to be c)
  exclude can be negative (which means, that no element of l is excluded)
*/
void bcp_DoBCLCofactorByCube(bcp p, bcl l, bc c, int exclude)
{
  int i;
  int b;
  bc lc;
  __m128i cc;
  __m128i dc;
  
  dc = _mm_loadu_si128(bcp_GetGlobalCube(p, 3));
  
  if ( exclude >= 0 )
    l->flags[exclude] = 1;
  
  for( b = 0; b < p->blk_cnt; b++ )
  {
    cc = _mm_andnot_si128( _mm_loadu_si128(c+b), dc);
    
    for( i = 0; i < l->cnt; i++ )
    {
      if ( l->flags[i] == 0 )
      {
        lc = bcp_GetBCLCube(p, l, i);
        _mm_storeu_si128(lc+b,  _mm_or_si128(cc, _mm_loadu_si128(lc+b)));
      }
    }
  }
  bcp_DoBCLSingleCubeContainment(p, l);
}

/*
  same as "bcp_DoBCLCofactorByCube()" but will not alter "l"
  instead a new list is returned, which must be freed with bcp_DeleteBCL()
*/
bcl bcp_NewBCLCofactorByCube(bcp p, bcl l, bc c, int exclude)
{
  bcl n = bcp_NewBCLByBCL(p, l);
  if ( n == NULL )
    return NULL;
  bcp_DoBCLCofactorByCube(p, n, c, exclude);
  return n;
}


/*
  Precondition: call to 
    void bcp_CalcBCLBinateSplitVariableTable(bcp p, bcl l) 

  return 0 if there is any variable which has one's and "zero's in the table
  otherwise this function returns 1

  16 bit version
*/
int bcp_IsBCLUnate(bcp p)
{
  int b;
  bc zero_cnt_cube[8];
  bc one_cnt_cube[8];
  __m128i z;
  __m128i o;

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
        
        
        

        z = _mm_cmpeq_epi8(_mm_loadu_si128(zero_cnt_cube[4]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
        o = _mm_cmpeq_epi8(_mm_loadu_si128(one_cnt_cube[4]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
                       // if ones and zeros are present, then both values are 0x00 and the reseult of the or is also 0x00
                      // this means, if there is any bit in c set to 0, then BCL is not unate and we can finish this procedure
        if ( _mm_movemask_epi8(_mm_or_si128(o, z)) != 0x0ffff )
          return 0;

        z = _mm_cmpeq_epi8(_mm_loadu_si128(zero_cnt_cube[5]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
        o = _mm_cmpeq_epi8(_mm_loadu_si128(one_cnt_cube[5]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
                       // if ones and zeros are present, then both values are 0x00 and the reseult of the or is also 0x00
                      // this means, if there is any bit in c set to 0, then BCL is not unate and we can finish this procedure
        if ( _mm_movemask_epi8(_mm_or_si128(o, z)) != 0x0ffff )
          return 0;

        z = _mm_cmpeq_epi8(_mm_loadu_si128(zero_cnt_cube[6]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
        o = _mm_cmpeq_epi8(_mm_loadu_si128(one_cnt_cube[6]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
                       // if ones and zeros are present, then both values are 0x00 and the reseult of the or is also 0x00
                      // this means, if there is any bit in c set to 0, then BCL is not unate and we can finish this procedure
        if ( _mm_movemask_epi8(_mm_or_si128(o, z)) != 0x0ffff )
          return 0;

        z = _mm_cmpeq_epi8(_mm_loadu_si128(zero_cnt_cube[7]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
        o = _mm_cmpeq_epi8(_mm_loadu_si128(one_cnt_cube[7]+b), _mm_setzero_si128());     // 0 cnt becomes 0xff and all other values become 0x00
                       // if ones and zeros are present, then both values are 0x00 and the reseult of the or is also 0x00
                      // this means, if there is any bit in c set to 0, then BCL is not unate and we can finish this procedure
        if ( _mm_movemask_epi8(_mm_or_si128(o, z)) != 0x0ffff )
          return 0;
        
        
  }
  return 1;
}

