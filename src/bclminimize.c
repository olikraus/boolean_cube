
/*

  bclminimize.c
  
  boolean cube list minimize algorithms


  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/


*/

#include "bc.h"
#include <assert.h>

/*
  try to reduce the total number of element in "l" via some heuristics.
  The reduction is done by:
    - Widen the existing cubes
    - Removing redundant cubes
  The minimization does not:
    - Create new cubes (primes) which may cover existing terms better
*/
void bcp_MinimizeBCLWithOffSet(bcp p, bcl l)
{
  bcl complement;
  logprint(2, "bcp_MinimizeBCLWithOffSet, initial bcl size=%d", l->cnt );
  bcp_DoBCLSingleCubeContainment(p, l);         // do an initial SCC simplification
  complement = bcp_NewBCLComplementWithSubtract(p, l);      // calculate the complement for the expand algo
  assert( complement != NULL );
  bcp_DoBCLExpandWithOffSet(p, l, complement);          // expand all terms as far es possible
  bcp_DeleteBCL(p, complement);
  bcp_DoBCLSingleCubeContainment(p, l);                         // do another SCC as a preparation for the MCC
  bcp_DoBCLMultiCubeContainment(p, l);                          // finally do multi cube minimization
  logprint(2, "bcp_MinimizeBCLWithOffSet, result bcl size=%d", l->cnt );
}

/* version with cofacter expand, but this seems to be SLOWER than the version with the off-set */
void bcp_MinimizeBCLWithOnSet(bcp p, bcl l)
{
  logprint(2, "bcp_MinimizeBCLWithOnSet, initial bcl size=%d", l->cnt );
  bcp_DoBCLSingleCubeContainment(p, l);         // do an initial SCC simplification
  bcp_DoBCLExpandWithCofactor(p, l);          // expand all terms as far es possible
  bcp_DoBCLSingleCubeContainment(p, l);                         // do another SCC as a preparation for the MCC
  bcp_DoBCLMultiCubeContainment(p, l);                          // finally do multi cube minimization
  logprint(2, "bcp_MinimizeBCLWithOnSet, result bcl size=%d", l->cnt );
}

void bcp_MinimizeBCLWithSubtract(bcp p, bcl l)
{
  bcl off_set;
  bcl result;
  logprint(2, "bcp_MinimizeBCLWithSubtract, initial bcl size=%d", l->cnt );
  off_set = bcp_NewBCLComplementWithSubtract(p, l); // includes bcp_DoBCLMultiCubeContainment
  result = bcp_NewBCLComplementWithSubtract(p, off_set); // includes bcp_DoBCLMultiCubeContainment
  bcp_CopyBCL(p, l, result);
  bcp_DeleteBCL(p, off_set);
  bcp_DeleteBCL(p, result);
  logprint(2, "bcp_MinimizeBCLWithSubtract, result bcl size=%d", l->cnt );
}

void bcp_MinimizeBCL(bcp p, bcl l)
{
  bcp_MinimizeBCLWithOnSet(p, l);
  // bcp_MinimizeBCLWithOffSet(p, l);
  //bcp_MinimizeBCLWithSubtract(p, l);
}


