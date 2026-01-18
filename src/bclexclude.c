/*

  bcexclude.c

  Boolean Cube Calculator
  (c) 2026 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

  Handle variables, which exclude each other

*/
#include "bc.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


/*============================================================*/
/*
  Background:
    There are cases, where variables do exclude each other.
    Let's assume a1, a2, a3, a4
    If the input clause is "a1", then actually it means "a1" but not a2, a3 and a4, so the correct
    expression would be "a1 & !a2 & !a3 & !a4"
    The same is true for "a1&a3": In this case the actual meaning is "a1 & !a2 & a3 & !a4"
	
	The opposite should be also considered:
	If the input is "!a1", then the others are probably wanted
	
	If will be difficult, if the user enters "!a1 & a2". Then the question is, what is the actual meaning? We can probably fallback to the first case.

    So the idea is: With all variables of a group of variables, where the variables are not used in the expression: Set those variables to "01" (zero)
    This requires a SOP representation in the BCL (which usually should be the case)

    If such a group is combined with other variables (which is usually the case), then the above "nullifing" should be applied.
    However if there is no such group, then nothing should happen.

    This leads to the following pseudo code:

    input: BCL, group of variables

    used var group = a list of all variables of group, which are used anywhere in the given BCL 
	
    if this list is not empty:
      if all vars in var group are negative then: (note: in the code below the if/else blocks are swapped)
        for each cube in the given BCL
          with all variables of the group, which are NOT part of used var group
            set the varibale to 10 in the current cube
      else
        for each cube in the given BCL
          with all variables of the group, which are NOT part of used var group
            set the varibale to 01 in the current cube
        
	Because the "one" assignment might also happen, the function name is misleading, because depending on the 
	original bcl, the dc vars are excluded or included.
	
	This is executed by the "group2zero0" command, which is also misleading, because there might be also a one assignment.
  
  
*/

int bcl_ExcludeBCLVars(bcp p, bcl l, bcl grp)
{
  bc l_var;
  bc grp_var;
  bc c;
  int is_any_var_used;
  int is_any_positive_var_used;
  int is_any_negative_var_used;
  int i, j;
  int grp_var_cnt = 0;
  __m128i mask;

  
  bcp_StartCubeStackFrame(p);
  l_var = bcp_GetTempCube(p);
  grp_var = bcp_GetTempCube(p);
  
  
  if ( l_var == NULL || grp_var == NULL )
    return bcp_EndCubeStackFrame(p), 0;

  /* get the unate/binate status of both lists */
  bcp_AndElementsBCL(p, l, l_var);
  bcp_AndElementsBCL(p, grp, grp_var);  // argument group is only used here --> we could use a single cube instead here

  //bcp_ShowBCL(p, grp);
  //bcp_ShowBCL(p, l);
  
  /* STEP 1: find out, whether any variables from grp_var are used in l_var */
  is_any_var_used = 0;
  is_any_positive_var_used = 0;
  is_any_negative_var_used = 0;
  for( i = 0; i < p->var_cnt; i++ )
  {
    if ( bcp_GetCubeVar(p, grp_var, i) != 3 )   // if the variable is in the group, then...
    {
      grp_var_cnt++;
      if ( bcp_GetCubeVar(p, l_var, i) != 3 ) // if the same variable is also used in the BCL
      {
        is_any_var_used = 1;
		if ( bcp_GetCubeVar(p, l_var, i) == 1 )
			is_any_negative_var_used = 1;
		if ( bcp_GetCubeVar(p, l_var, i) == 2 )
			is_any_positive_var_used = 1;
      }
    }
  }
  
  if ( is_any_var_used == 0 )           
  {
    logprint(2, "bcl_ExcludeBCLVars: No group variable used (%d vars in the group)", grp_var_cnt);

    return bcp_EndCubeStackFrame(p), 1; // none of the variables of the group are used, so keep the BCL as it is
  }
  logprint(2, "bcl_ExcludeBCLVars: Some group variables used (%d vars in the group), is_positive=%d, is_negative=%d", grp_var_cnt, is_any_positive_var_used, is_any_negative_var_used);
    
	
  if ( is_any_positive_var_used )
  {
		
	  /* STEP 2: Calculate "and" mask */
	  /* it this point, some variables of the group are used */
	  /* goal is now to force variables in BCL to "01", which are previoulsy not used in the BCL ("11") */
	  /* for this goal, we will reuse grp_var as a mask for the BCL: */
	  /* group vars, which are used in the BCL, will become "11", leaving those variables untouched */
	  /* group vars, which are not used in the BCL, will become "01", forcing those vars to zero ("01") */
	  for( i = 0; i < p->var_cnt; i++ )
	  {
		if ( bcp_GetCubeVar(p, grp_var, i) != 3 )   // if the variable is in the group, then...
		{
		  if ( bcp_GetCubeVar(p, l_var, i) != 3 ) // if the same variable is also used in the BCL
		  {
			bcp_SetCubeVar(p, grp_var, i, 3);       // the group var is used in BCL, so revert this to DC
		  }
		  else
		  {
			bcp_SetCubeVar(p, grp_var, i, 1);       // the group var is not used in BCL, so force the value to "01"
		  }
		}
	  }
	  
	  /* STEP 3: apply "and" mask */
	  /* now the mask is inside grp_var and we can just "and" the mask with all cubes in BCL */
	  for( j = 0; j < p->blk_cnt; j++ )
	  {
		mask = _mm_loadu_si128(grp_var+j);  // grp_var now contains the mask for the BCL cubes
		for( i = 0; i < l->cnt; i++ )
		{
		  c = bcp_GetBCLCube(p,l,i);
		  _mm_storeu_si128(c+j, _mm_and_si128(_mm_loadu_si128(c+j), mask)); // force the unused vars from the group to zero
		}
	  }
  }
  else if ( is_any_negative_var_used ) // && is_any_positive_var_used == 0 
  {
	  /* STEP 2: Calculate "and" mask */
	  /* it this point, some variables of the group are used BUT all of them are used negative */
	  /* goal is now to force variables in BCL to "10" (positive), which are previoulsy not used in the BCL ("11") */
	  /* for this goal, we will reuse grp_var as a mask for the BCL: */
	  /* group vars, which are used in the BCL, will become "11", leaving those variables untouched */
	  /* group vars, which are not used in the BCL, will become "10", forcing those vars to one ("10") */
	  for( i = 0; i < p->var_cnt; i++ )
	  {
		if ( bcp_GetCubeVar(p, grp_var, i) != 3 )   // if the variable is in the group, then...
		{
		  if ( bcp_GetCubeVar(p, l_var, i) != 3 ) // if the same variable is also used in the BCL
		  {
			bcp_SetCubeVar(p, grp_var, i, 3);       // the group var is used in BCL, so revert this to DC
		  }
		  else
		  {
			bcp_SetCubeVar(p, grp_var, i, 2);       // the group var is not used in BCL, so force the value to "10"
		  }
		}
	  }
	  
	  /* STEP 3: apply "and" mask */
	  /* now the mask is inside grp_var and we can just "and" the mask with all cubes in BCL */
	  for( j = 0; j < p->blk_cnt; j++ )
	  {
		mask = _mm_loadu_si128(grp_var+j);  // grp_var now contains the mask for the BCL cubes
		for( i = 0; i < l->cnt; i++ )
		{
		  c = bcp_GetBCLCube(p,l,i);
		  _mm_storeu_si128(c+j, _mm_and_si128(_mm_loadu_si128(c+j), mask)); // force the unused vars from the group to one
		}
	  }
		
  }

  bcp_EndCubeStackFrame(p);
  return 1;
}


/*
  Update Jan 2025
      
    The case "a1&a3" should not exist. a1 and a2 exclude each other, so the cube is invalid and should be removed.
    The user probably wanted to have a1|a3 which would then lead to two separate cubes.
    If "a1&a3" has been created by an intersection calculation, then removing this cube is probably correct.
    
    The case "!a1 & a2" probably depends, whether the bcl is part of an on-set or an off-set.
    On-Set: Exclude all dc variables
    
    The case "!a1" actually means "a2|a3|a4", which then has to be expanded to
      !a1&a2&!a3&!a4
      !a1&!a2&a3&!a4
      !a1&!a2&!a3&a4
    In general (for example for !a1&!a2) we could express this by a3 and a4
      !a1&!a2&a3&!a4
      !a1&!a2&!a3&a4
  
    Consider 0n-Set only
      Case 0: All member of the group are dc --> DONE
      Case 1: Two or more positive group members --> REMOVE the cube
      Case 2: Exactly one positive group member (other group member can be DC or negative)-->  SET other group member to negative
      Case 3: Only DC and negative group members: Probably similar to sharp operation:
        For each DC group member, create a new cube, where this DC group member is positive and all others are negative (Case 2)
        The remove the cube with the negative only group members.

    Off-Set: ?
      Maybe not required, the on set case seems to be symetric anyways
    
  Technical implementation
      bcp_GetVariableMask(bcp p, bc mask, bc c)         create 00 (illegal) and 01 (zeros) for the variables
        hmm probably we need 11 for member variables and 00 for all other
        
    Input: 
      gmc group member cube: 10 for all member variables, 11 for anything else
      c         any cube of a bcl
      
      mask = get mask from gmc
      masked_c = c AND mask
      one_cnt = get all ones from masked_c
      zero_cnt = get all zeros from masked_c
      
      if ( one_cnt == 0 and zero_cnt == 0 )
        return 1                // case 0, no todo
      if ( one_cnt >= 2 )
        REMOVE c                // case 1
      if ( one_cnt == 1 )
        see code from prev function, // case 2
      // case 3:
        REPLACE c with member-cnt - zero_cnt new cubes
        
*/
/*
  p: problem structure
  l: the list, on which we apply the group exclusion
  idx:  the cube within l to which we apply the exclusion
  grp_dc_mask:      must contain only 00 (for none-group members) and 11 (for group members)
*/
int bcp_DoBCLCubeExcludeGroup(bcp p, bcl l, int idx, bc grp_dc_mask)
{
  bc cube = bcp_GetBCLCube(p,l,idx);
  __m128i zero_mask = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 1));       // idx 0: all illegal (00), idx 1: all zero (01), idx 2: all one (10) and idx 3: all don't care (11)
  __m128i one_mask = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 2));       // idx 0: all illegal (00), idx 1: all zero (01), idx 2: all one (10) and idx 3: all don't care (11)  
  __m128i r;
  __m128i z;
  __m128i o;
  unsigned j;
  unsigned zero_cnt;
  unsigned one_cnt;
  int one_pos = -1;          // position of the first 10 variable in the cube

  printf("bcp_DoBCLCubeExcludeGroup: cube=%s\n", bcp_GetStringFromCube(p, cube));
  
  zero_cnt = 0;
  one_cnt = 0;
  for( j = 0; j < p->blk_cnt; j++ )
  {
    /*
      task: count 01 (zeros) and 10 (once).
      idea: instead of counting the 1 we do count the 0, this means we have to force all none-group variables to dc, this is done in the next step:
    */
    r = _mm_or_si128( _mm_xor_si128(_mm_loadu_si128(grp_dc_mask+j), _mm_set1_epi32(-1)), _mm_loadu_si128(cube+j) );    // get the blk from memory and force all none-group var's to dc
    z = _mm_or_si128( r, zero_mask );           // or with the zero mask   10 (one) | 01 --> 11 (dc)   01 (zero) | 01 --> 01 (zero)
    o = _mm_or_si128( r, one_mask );           // or with the zero mask   10 (one) | 10 --> 10 (one)   01 (zero) | 10 --> 11 (dc)

    /* count 0 to get the number of 01 variables */
    zero_cnt += __builtin_popcountll(~_mm_cvtsi128_si64(_mm_unpackhi_epi64(z, z)));
    zero_cnt += __builtin_popcountll(~_mm_cvtsi128_si64(z));
    
    /* count 0 to get the number of 10 variables */
    one_cnt += __builtin_popcountll(~_mm_cvtsi128_si64(_mm_unpackhi_epi64(o, o)));
    if ( one_cnt > 0 && one_pos < 0 )
    {
        // calculate the position of the one for the upper half part of the __m128i, only required for case 2
        one_pos = __builtin_ctzll(~_mm_cvtsi128_si64(_mm_unpackhi_epi64(z, z))) / 2  + p->vars_per_blk_cnt/2 + j*p->vars_per_blk_cnt; 
    }
    one_cnt += __builtin_popcountll(~_mm_cvtsi128_si64(o));
    if ( one_cnt > 0 && one_pos < 0 )
    {
        one_pos = __builtin_ctzll(~_mm_cvtsi128_si64(o)) / 2 + j*p->vars_per_blk_cnt;
    }
  }

  printf("bcp_DoBCLCubeExcludeGroup: zero_cnt=%d one_cnt=%d one_pos=%d\n", zero_cnt, one_cnt, one_pos);
  
  if ( one_cnt == 0 && zero_cnt == 0 )
  {
  }
  else if ( one_cnt >= 2 )
  {
    // REMOVE cube                // case 1
    l->flags[idx] = 1;      // mark the cube as deleted
  }
  else if ( one_cnt == 1 )
  {
    /*
      case 2
      there is only one positive (one) variable, so mark all other group members as zero
      in the dc mask, we will remove that one variable and set it to illegal, then we assign the zero to the other members and finally revert the change in the dc mask
    */
    assert( bcp_GetCubeVar(p, cube, one_pos) == 2 );             // crosscheck, that our calculation had been correct... 2 is the code for 10, which is the one 
    assert( bcp_GetCubeVar(p, grp_dc_mask, one_pos) == 3 );             // crosscheck, that our calculation had been correct... 3 is the dc mark for the group member variables
    bcp_SetCubeVar(p, grp_dc_mask, one_pos, 0);                 // remove the one from the member list by unmarking it
    for( j = 0; j < p->blk_cnt; j++ )
    {
      r = _mm_xor_si128(_mm_loadu_si128(grp_dc_mask+j), _mm_set1_epi32(-1));           // get the inverted dc mask
      r = _mm_or_si128( r, zero_mask );         // now we have 11 for all none group vars (and the single one) and 01 for all other member variables
      r = _mm_and_si128( r, _mm_loadu_si128(cube+j)  );         // force all other member variables to 01, actually this is only required for 11 variables, but check for this is too expensive
    _mm_storeu_si128(cube+j, r);          // store the result in the destination cube    
    bcp_SetCubeVar(p, grp_dc_mask, one_pos, 3);                 // undo the modification from above
    }
  }
  else  // one_cnt==0 && zero_cnt > 0
  {
    /*
      case 3
      for all dc variables in the member list, create a new cube in l, which is a copy of the origianl cube, but the dc variable is set to 10
      this will lead to a new cube, which fullfills case 2, which is indeed executed later, because the cube is added to the list
    */
    for( j = 0; j < p->var_cnt; j++ )
    {
      if ( bcp_GetCubeVar(p, grp_dc_mask, j) == 3 ) // is this a group member var?
      {
        if ( bcp_GetCubeVar(p, cube, j) == 3 ) // is this dc in the cube?
        {
          int new_cube_pos =  bcp_AddBCLCubeByCube(p, l, cube);
          if ( new_cube_pos < 0 )
            return 0;           // memory error
          bcp_SetCubeVar(p, bcp_GetBCLCube(p,l,new_cube_pos), j, 2); // replace the dc with a one
        }
      }
    }
  }
  return 1;
}


/*
  p: problem structure
  l: the list, on which we apply the group exclusion
  grp:      a cube which contains 10 for all group members and 11 for all others
*/
int bcp_DoBCLExcludeGroup(bcp p, bcl l, bc grp)
{
  bc grp_dc_mask;
  int j;
  __m128i r;
  
  bcp_StartCubeStackFrame(p);
  grp_dc_mask = bcp_GetTempCube(p);  // goal is to create a new cube with 00 for not member and 11 for member variables
  for( j = 0; j < p->blk_cnt; j++ )
  {
    r = _mm_loadu_si128(grp+j); // only contains 11 and 10
    r = _mm_xor_si128(r, _mm_set1_epi32(-1));  // bit invert, so we have 00 and 01
    r = _mm_or_si128( r,   _mm_slli_epi16 (r, 1)); // create 00 and 11 
    _mm_storeu_si128(grp_dc_mask+j, r);          // store the result in the mask cube    
  }

  coPrint(p->var_map); puts("");
  bcp_ShowBCL(p, l);
  printf("bcp_DoBCLExcludeGroup: grp_dc_mask=%s\n", bcp_GetStringFromCube(p, grp_dc_mask));
  
  
  for( j = 0; j < l->cnt; j++ )
  {
    if ( l->flags[j] == 0 )
    {
      if ( bcp_DoBCLCubeExcludeGroup(p, l, j, grp_dc_mask) == 0 )  // this may modify l->cnt
      {
        bcp_EndCubeStackFrame(p);
        return 0;         // memory error
      }
    }
  }
  
  bcp_PurgeBCL(p, l);  // physically remove cubes, which are marked as deleted
  
  bcp_EndCubeStackFrame(p);
  return  1;
}
