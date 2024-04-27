/*

  bclsubtract.c
  
  boolean cube list: substract / difference calculation

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/



  this file also contains the "sharp" which does the substract on cube level
  
  The core function is the bcp_DoBCLSharpOperation() which
  calculations a-b and adds the result to a given list "l"
  
  The substract algo can be used to
    - check whether a bcl is a subset of another bcl
    - calculate the complement of a given bcl
    
  In both cases there are alternative algorithms available
    - the subset test can be done by a subset check for each cube (via tautology test)
    - the calculation of the complement can be done via a cofactor split as
        long as there is a binate variable. 
  
*/

#include "bc.h"
#include <assert.h>

/*
  Subtract cube b from a: a#b. All cubes resulting from this operation are appended to l if
    - the new cube is valid
    - the new cube is not covered by any other cube in l
  After adding a cube, existing cubes are checked to be a subset of the newly added cube and marked for deletion if so
*/
static int bcp_DoBCLSharpOperation(bcp p, bcl l, bc a, bc b)
{
  int i;
  unsigned bb;
  unsigned orig_aa;
  unsigned new_aa;
  //int pos;

  for( i = 0; i < p->var_cnt; i++ )
  {
    bb = bcp_GetCubeVar(p, b, i);
    if ( bb != 3 )
    {
      orig_aa = bcp_GetCubeVar(p, a, i); 
      new_aa = orig_aa & (bb^3);
      if ( new_aa != 0 )
      {
        bcp_SetCubeVar(p, a, i, new_aa);        // modify a 
        if ( bcp_AddBCLCubeByCube(p, l, a) < 0 ) // if a is not subcube of any existing cube, then add the modified a cube to the list
          return 0;  // memory error
        bcp_SetCubeVar(p, a, i, orig_aa);        // undo the modification
      }
    }
  }
  return 1; // success
}

/* 
  a = a - b 
  is_mcc: whether to execute multi cube containment or not
  if is_mcc is 0, then the substract operation will generate all prime cubes.
  if b is unate, then executing mcc slows down the substract, otherwise if b is binate, then using mcc increases performance
*/
int bcp_SubtractBCL(bcp p, bcl a, bcl b, int is_mcc)
{
  int i, j;
  bcl result = bcp_NewBCL(p);
  if ( result == NULL )
    return 0;
  for( i = 0; i < b->cnt; i++ )
  {
    bcp_ClearBCL(p, result);
    for( j = 0; j < a->cnt; j++ )
    {
      if ( bcp_DoBCLSharpOperation(p, result, bcp_GetBCLCube(p, a, j), bcp_GetBCLCube(p, b, i)) == 0 )
        return bcp_DeleteBCL(p, result), 0;
    }
    if ( bcp_CopyBCL(p, a, result) == 0 )
        return bcp_DeleteBCL(p, result), 0;
    bcp_DoBCLSingleCubeContainment(p, a);
    if ( is_mcc )
      bcp_DoBCLMultiCubeContainment(p, a);
  }
  bcp_DeleteBCL(p, result);
  return 1; // success
}

