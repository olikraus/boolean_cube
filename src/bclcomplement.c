/*

  bclcomplement.c
  
  boolean cube list: complement calculation
  
  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/


  There are two ways to calculate the complement
    1) Via substract from the tautology cube
    2) Via recursiv split
    
  This code defines both algorithms, but at the moment the substract from the
  tautology cube seems to be faster.
  
  This code defines also 
    bcl bcp_NewBCLComplement(bcp p, bcl l)
  which just calles "bcp_NewBCLComplementWithSubtract"
  
*/
#include "bc.h"
#include <assert.h>

/*=================================================================*/
/* complement with subtract */


/*
  looks like the complement with substract is much faster. than the cofactor version
*/
bcl bcp_NewBCLComplementWithSubtract(bcp p, bcl l)
{
    bcl result = bcp_NewBCL(p);
    int is_mcc = 1;
    if ( result == NULL )
      return NULL;

    logprint(2, "bcp_NewBCLComplementWithSubtract, bcl size=%d", l->cnt );
    
    bcp_CalcBCLBinateSplitVariableTable(p, l);
    if ( bcp_IsBCLUnate(p) )
      is_mcc = 0;
    if ( bcp_AddBCLCubeByCube(p, result, bcp_GetGlobalCube(p, 3)) < 0)  // 3: universal cube
      return bcp_DeleteBCL(p, result), NULL;
    bcp_SubtractBCL(p, result, l, is_mcc);             // "result" contains the negation of "l"
    
    // do a small minimization step
    bcp_DoBCLExpandWithOffSet(p, result, l);   // not sure whether this will help, cubes might be already max due to the sharp operation
    

    bcp_DoBCLMultiCubeContainment(p, result);

    return result;
}

bcl bcp_NewBCLComplement(bcp p, bcl l)
{
  //return bcp_NewBCLComplementWithIntersection(p, l);
  return bcp_NewBCLComplementWithSubtract(p, l);
}

int bcp_ComplementBCL(bcp p, bcl l)
{
  bcl ll = bcp_NewBCLComplement(p, l);
  if ( ll == NULL )
    return 0;
  if ( bcp_CopyBCL(p, l, ll) == 0 )
    return bcp_DeleteBCL(p, ll), 0;  
  return bcp_DeleteBCL(p, ll), 1;
}

/*=================================================================*/
/* complement with cofactor */

/*
  complement calculation with cofactor seems to be slower...
*/
bcl bcp_NewBCLComplementWithCofactorSub(bcp p, bcl l)
{
  int var_pos;
  int i, j;
  bcl f1;
  bcl f2;
  bcl cf1;
  bcl cf2;
 
  /*
  if ( l->cnt <= 14 )
  {
    return bcp_NewBCLComplementWithSubtract(p, l);
  }
  */
    
  
  bcp_CalcBCLBinateSplitVariableTable(p, l);
  var_pos = bcp_GetBCLMaxBinateSplitVariable(p, l);
  if ( var_pos < 0 )
  {
    bcl result = bcp_NewBCL(p);
    bcp_AddBCLCubeByCube(p, result, bcp_GetGlobalCube(p, 3));  // "result" contains the universal cube
    bcp_SubtractBCL(p, result, l, 0);             // "result" contains the negation of "l"
    return result;
    /* return bcp_NewBCLComplementWithSubtract(p, l); */
  }
  
  f1 = bcp_NewBCLCofacterByVariable(p, l, var_pos, 1);
  assert(f1 != NULL);
  bcp_DoBCLSimpleExpand(p, f1);
  //bcp_DoBCLSingleCubeContainment(p, f1);
  
  f2 = bcp_NewBCLCofacterByVariable(p, l, var_pos, 2);
  assert(f2 != NULL);
  bcp_DoBCLSimpleExpand(p, f2);
  //bcp_DoBCLSingleCubeContainment(p, f2);
  
  cf1 = bcp_NewBCLComplementWithCofactorSub(p, f1);
  assert(cf1 != NULL);
  cf2 = bcp_NewBCLComplementWithCofactorSub(p, f2);
  assert(cf2 != NULL);
  
  for( i = 0; i < cf1->cnt; i++ )
    if ( cf1->flags[i] == 0 )
      bcp_SetCubeVar(p, bcp_GetBCLCube(p, cf1, i), var_pos, 2);  
  //bcp_DoBCLSimpleExpand(p, cf1);
  bcp_DoBCLSingleCubeContainment(p, cf1);

  for( i = 0; i < cf2->cnt; i++ )
    if ( cf2->flags[i] == 0 )
      bcp_SetCubeVar(p, bcp_GetBCLCube(p, cf2, i), var_pos, 1);  
  //bcp_DoBCLSimpleExpand(p, cf2);
  bcp_DoBCLSingleCubeContainment(p, cf2);

  /*
    at this point the merging process should be much smarter
    at least we need to check, whether cubes can be recombined with each other
  */

  /* do some simple merge, if the cube just differs in the selected variable */
  for( i = 0; i < cf2->cnt; i++ )
  {
    if ( cf2->flags[i] == 0 )
    {
      bc c = bcp_GetBCLCube(p, cf2, i);
      bcp_SetCubeVar(p, c, var_pos, 2);  
      for( j = 0; j < cf1->cnt; j++ )
      {
	if ( i != j )
	{
	  if ( bcp_CompareCube(p, c, bcp_GetBCLCube(p, cf1, j)) == 0 )
	  {
	    // the two cubes only differ in the selected variable, so extend the cube in cf1 and remove the cube from cf2
	    bcp_SetCubeVar(p, bcp_GetBCLCube(p, cf1, j), var_pos, 3);
	    cf2->flags[i] = 1;  // remove the cube from cf2, so that it will not be added later by bcp_AddBCLCubesByBCL()
	  }
	} // i != j
      } // for cf1
      bcp_SetCubeVar(p, c, var_pos, 1);  // undo the change in the cube from cf2
    }
  }

  bcp_DeleteBCL(p, f1);
  bcp_DeleteBCL(p, f2);
  
  if ( bcp_AddBCLCubesByBCL(p, cf1, cf2) == 0 )
    return  bcp_DeleteBCL(p, cf1), bcp_DeleteBCL(p, cf2), NULL;

  bcp_DeleteBCL(p, cf2);

  //bcp_DoBCLSimpleExpand(p, cf1);
  bcp_DoBCLExpandWithOffSet(p, cf1, l);
  bcp_DoBCLSingleCubeContainment(p, cf1);
  
  return cf1;
}

/*
  better use "bcp_NewBCLComplementWithSubtract()"
*/
bcl bcp_NewBCLComplementWithCofactor(bcp p, bcl l)
{
  bcl n;
  logprint(2, "bcp_NewBCLComplementWithCofactor, bcl size=%d", l->cnt );
  n = bcp_NewBCLComplementWithCofactorSub(p, l);
  bcp_DoBCLMultiCubeContainment(p, n);
  return n;
}


/*=================================================================*/
/* complement with demorgan & intersection */

void bcp_InvertCubesBCL(bcp p, bcl l)
{
  int i;
  for( i = 0; i < l->cnt; i++ )
    bcp_InvertCube(p, bcp_GetBCLCube(p, l, i));
}

/*
	create a cube with a signle varibale at "pos' with value 'val'
*/
void bcp_GetSingleVarCube(bcp p, bc dest, int pos, int val)
{
  bcp_CopyGlobalCube(p, dest, 3);	// copy dc cube to dest
  bcp_SetCubeVar(p, dest, pos, val);		// set the variable
}

/*
	create a bcl with one cube for each positive or negative variable
*/
int bcp_AddBCLCubesWithSingleVariables(bcp p, bcl result, bc src_cube)
{
  int i;
  int val;
  int cube_pos;
  if ( result == NULL )
    return 0;
  for( i = 0; i < p->var_cnt; i++ )
  {
    val = bcp_GetCubeVar(p, src_cube, i);
    if ( val < 3 )
    {
      cube_pos = bcp_AddBCLCube(p, result);
      if ( cube_pos < 0 )
	return 0;
      bcp_GetSingleVarCube(p, bcp_GetBCLCube(p, result, cube_pos), i, val);
    }
  }
  return 1;  
}

/*
  special version for 
    int bcp_IntersectionBCLs(bcp p, bcl result, bcl a, bcl b)
*/
int bcp_IntersectionBCLsForComplement(bcp p, bcl result, bcl a, bcl b, bcl offSet)
{
  int i, j;
  bc tmp;
  
  bcp_StartCubeStackFrame(p);
  tmp = bcp_GetTempCube(p);
  
  assert(result != a);
  assert(result != b);
  
  bcp_ClearBCL(p, result);
  for( i = 0; i < b->cnt; i++ )
  {
    for( j = 0; j < a->cnt; j++ )
    {
      if ( bcp_IntersectionCube(p, tmp, bcp_GetBCLCube(p, a, j), bcp_GetBCLCube(p, b, i)) )
      {
        if ( bcp_AddBCLCubeByCube(p, result, tmp) < 0 )
          return bcp_EndCubeStackFrame(p), 0;
      }
    }
  }
  
  bcp_DoBCLSingleCubeContainment(p, result);
  bcp_DoBCLExpandWithOffSet(p, result, offSet);


  bcp_EndCubeStackFrame(p);  
  return 1;
}

int bcp_IntersectionBCLForComplement(bcp p, bcl a, bcl b, bcl offSet)
{
  bcl result = bcp_NewBCL(p);
  
  if ( bcp_IntersectionBCLsForComplement(p, result, a, b, offSet) == 0 )
    return bcp_DeleteBCL(p, result), 0;
  
  bcp_CopyBCL(p, a, result);
  bcp_DeleteBCL(p, result);
  return 1;
}


/*
  apply distribution law
*/
bcl bcp_NewBCLVariableIntersection(bcp p, bcl l, bcl offSet)
{
  bcl result = bcp_NewBCL(p);
  bcl cl;
  int i;
  if ( result == NULL )
    return NULL;
  cl = bcp_NewBCL(p);
  if ( cl == NULL )
    return bcp_DeleteBCL(p, result), NULL;
  if ( l->cnt == 0 )
    return result;		// not clear whther this is the correct value
  if ( bcp_AddBCLCubesWithSingleVariables(p, cl, bcp_GetBCLCube(p, l, 0)) == 0 )
    return bcp_DeleteBCL(p, result), bcp_DeleteBCL(p, cl), NULL;
  bcp_AddBCLCubesByBCL(p, result, cl );
  for( i = 1; i < l->cnt; i++ )
  {
    bcp_ClearBCL(p, cl);
    if ( bcp_AddBCLCubesWithSingleVariables(p, cl, bcp_GetBCLCube(p, l, i)) == 0 )
      return bcp_DeleteBCL(p, result), bcp_DeleteBCL(p, cl), NULL;
    if ( bcp_IntersectionBCLForComplement(p, result, cl, offSet) == 0 )
      return bcp_DeleteBCL(p, result), bcp_DeleteBCL(p, cl), NULL;
    logprint(4, "bcp_NewBCLVariableIntersection, step %d/%d, result size %d", i+1, l->cnt, result->cnt );
  }
  return bcp_DeleteBCL(p, cl), result;
}



bcl bcp_NewBCLComplementWithIntersection(bcp p, bcl l)
{
  bcl result;
  bcl ll = bcp_NewBCLByBCL(p, l);
  if ( ll == NULL )
    return NULL;
  logprint(2, "bcp_NewBCLComplementWithIntersection, bcl size=%d", l->cnt );
  
  bcp_InvertCubesBCL(p, ll);
  result = bcp_NewBCLVariableIntersection(p, ll, l);
  //if ( result != NULL )
  //  bcp_DoBCLMultiCubeContainment(p, result);
  //if ( result != NULL )
  //  bcp_MinimizeBCL(p, result);
  bcp_DeleteBCL(p, ll);
  return result;
}

