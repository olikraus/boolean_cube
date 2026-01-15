/*

  bclcontainment.c
  
  boolean cube list: containment

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

*/

#include "bc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/*============================================================*/
/* simple single covered procedure */

int bcp_IsBCLCubeSingleCovered(bcp p, bcl l, bc c)
{
  int i;
  int cnt = l->cnt;
  for( i = 0; i < cnt; i++ )
  {
    if ( l->flags[i] == 0 )
    {
      if ( bcp_IsSubsetCube(p, bcp_GetBCLCube(p, l, i), c) != 0 )       // bcube.c:bcp_IsSubsetCube
        return 1;  // yes, c is a subset of bcp_GetBCLCube(p, l, i)
    }
  }
  return 0;
}

/*
  In the given BCL, ensure, that no cube is part of any other cube
  This will call bcp_PurgeBCL()
*/
void bcp_DoBCLSingleCubeContainment(bcp p, bcl l)
{
  int i, j;
  int cnt = l->cnt;
  bc c;
  int vc;
  int reduceCnt = 0;
  
  /*
    calculate the number of 01 and 10 codes for each of the cubes in "l"
    idea is to reduce the number of "subset" tests, because for a cube with n variables,
    can be a subset of another cube only if this other cube has lesser variables.
    however: also the equal case is checked, this means to check whether two cubes are identical
  */
  int *vcl = bcp_GetBCLVarCntList(p, l);

  
  for( i = 0; i < cnt; i++ )
  {
    if ( l->flags[i] == 0 )
    {
      c = bcp_GetBCLCube(p, l, i);
      vc = vcl[i];
      for( j = 0; j < cnt; j++ )
      {
        if ( l->flags[j] == 0 )
        {
          if ( i != j )
          {
            /*
              test, whether "b" is a subset of "a"
              returns:      
                1: yes, "b" is a subset of "a"
                0: no, "b" is not a subset of "a"
              A mandatory condition for this is, that b has more variables than a
            */
            if ( vcl[j] >= vc )
            {
              if ( bcp_IsSubsetCube(p, c, bcp_GetBCLCube(p, l, j)) != 0 )       // bcube.c:bcp_IsSubsetCube
              {
                l->flags[j] = 1;      // mark the j cube as deleted
		reduceCnt++;
              }
            }
          } // j != i
        } // j cube not deleted
      } // j loop
    } // i cube not deleted
  } // i loop
  bcp_PurgeBCL(p, l);
  free(vcl);
  logprint(8, "bcp_DoBCLSingleCubeContainment, reduceCnt=%d, bcl size=%d", reduceCnt, l->cnt);  
}

/*============================================================*/
/* complex cover, based on the idea, that the a cube is covered by a list, */
/* if the cofactor of the list against that cube has the tautology property */

/*
  checks, whether the given cube "c" is a subset (covered) of the list "l"
  cube "c" must not be physical part of the list. if this is the case use bcp_IsBCLCubeRedundant() instead
  
*/
int bcp_IsBCLCubeCovered(bcp p, bcl l, bc c)
{
  bcl n = bcp_NewBCLCofactorByCube(p, l, c, -1);
  int result = bcp_IsBCLTautology(p, n);
  bcp_DeleteBCL(p, n);
  return result;
}

/*
  checks, whether the cube at position "pos" is a subset of all other cubes in the same list.
*/
int bcp_IsBCLCubeRedundant(bcp p, bcl l, int pos)
{
  bcl n = bcp_NewBCLCofactorByCube(p, l, bcp_GetBCLCube(p, l, pos), pos);
  int result = bcp_IsBCLTautology(p, n);
  bcp_DeleteBCL(p, n);
  return result;
}

/*

  IRREDUNDANT 

  Remove cubes from "l", whch are covered by the rest of the list "l".

  This procedure will remove the smallest cubes first

*/
void bcp_DoBCLMultiCubeContainment(bcp p, bcl l)
{
  int i;
  int *vcl = bcp_GetBCLVarCntList(p, l);
  int min = p->var_cnt;
  int max = 0;
  int vc;
  int reduceCnt = 0;
  int step = 1;
  clock_t t0, t1;

  logprint(5, "bcp_DoBCLMultiCubeContainment start, bcl size=%d", l->cnt);  
  t1 = t0 = clock();

  for( i = 0; i < l->cnt; i++ )
  {
    if ( l->flags[i] == 0 )
    {
      if ( min > vcl[i] )
        min = vcl[i];
      if ( max < vcl[i] )
        max = vcl[i];
    }
  }

  for( vc = max; vc >= min; vc-- )      // it seems to be faster to start checking the smallest cubes first
  {
    for( i = 0; i < l->cnt; i++ )
    {
      if ( l->flags[i] == 0 && vcl[i] == vc )
      {
        if ( bcp_IsBCLCubeRedundant(p, l, i) )
        {
          l->flags[i] = 1;
	  reduceCnt++;
        }
	t1 = clock();
	logprint(6, "bcp_DoBCLMultiCubeContainment, step %d/%d, varcnt=%d [%d, %d], reduce cnt=%d, clock %lu/%lu", step, l->cnt, vc, min, max, reduceCnt, t1-t0, p->clock_do_bcl_multi_cube_containment); 
	step++;
	if ( t1-t0 > p->clock_do_bcl_multi_cube_containment )
	  break;
      } // i cube not deleted
      if ( t1-t0 > p->clock_do_bcl_multi_cube_containment )
        break;
    } // i loop
    if ( t1-t0 > p->clock_do_bcl_multi_cube_containment )
      break;
  } // vc loop
  free(vcl);
  bcp_PurgeBCL(p, l);
  logprint(5, "bcp_DoBCLMultiCubeContainment end, reduceCnt=%d, bcl size=%d, steps %d, time-limit reached=%d", reduceCnt, l->cnt, step, t1-t0 > p->clock_do_bcl_multi_cube_containment);  
}

