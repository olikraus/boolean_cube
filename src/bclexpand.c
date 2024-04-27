/*

  bclexpand.c
  
  boolean cube list: expand cubes

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

*/
#include "bc.h"
#include <assert.h>

/*
  try to expand cubes into another cube
  includes bcp_DoBCLSingleCubeContainment
*/
void bcp_DoBCLSimpleExpand(bcp p, bcl l)
{
  int i, j, v;
  int k;
  int cnt = l->cnt;
  int delta;
  int cval, dval;
  bc c, d;
  for( i = 0; i < cnt; i++ )
  {
    if ( l->flags[i] == 0 )
    {
      c = bcp_GetBCLCube(p, l, i);
      for( j = i+1; j < cnt; j++ )
      {
        if ( l->flags[j] == 0 )
        {
          //if ( i != j )
          {
            d = bcp_GetBCLCube(p, l, j); 
            delta = bcp_GetCubeDelta(p, c, d);
            //printf("delta=%d\n", delta);
            
            if ( delta == 1 )
            {
              // search for the variable, which causes the delta
              for( v = 0; v < p->var_cnt; v++ )
              {
                cval = bcp_GetCubeVar(p, c, v);
                dval = bcp_GetCubeVar(p, d, v);
                if ( (cval & dval) == 0 )
                {
                  break;
                }
              }
              if ( v < p->var_cnt )
              {
                //printf("v=%d\n", v);
                bcp_SetCubeVar(p, c, v, 3-cval);
                /*
                  test, whether second arg is a subset of first
                  returns:      
                    1: yes, second arg is a subset of first arg
                    0: no, second arg is not a subset of first arg
                */
                if ( bcp_IsSubsetCube(p, d, c) )
                {
                  // great, expand would be successful
                  bcp_SetCubeVar(p, c, v, 3);  // expand the cube, by adding don't care to that variable
                  //printf("v=%d success c\n", v);

                  /* check whether other cubes are covered by the new cube */
                  for( k = 0; k < cnt; k++ )
                  {
                    if ( k != j && k != i && l->flags[k] == 0)
                    {
                      if ( bcp_IsSubsetCube(p, c, bcp_GetBCLCube(p, l, k)) != 0 )
                      {
                        l->flags[k] = 1;      // mark the k cube as deleted
                      }
                    }
                  } // for k
                }
                else
                {
                  bcp_SetCubeVar(p, c, v, cval);  // revert variable
                  bcp_SetCubeVar(p, d, v, 3-dval);
                  if ( bcp_IsSubsetCube(p, c, d) )
                  {
                    // expand of d would be successful
                    bcp_SetCubeVar(p, d, v, 3);  // expand the d cube, by adding don't care to that variable
                    //printf("v=%d success d\n", v);
                    for( k = 0; k < cnt; k++ )
                    {
                      if ( k != j && k != i && l->flags[k] == 0)
                      {
                        if ( bcp_IsSubsetCube(p, d, bcp_GetBCLCube(p, l, k)) != 0 )
                        {
                          l->flags[k] = 1;      // mark the k cube as deleted
                        }
                      }
                    } // for k
                  }
                  else
                  {
                    bcp_SetCubeVar(p, d, v, dval);  // revert variable
                  }
                }
              }
            }
          } // j != i
        } // j cube not deleted
      } // j loop
    } // i cube not deleted
  } // i loop
  bcp_PurgeBCL(p, l);
}

/* OBSOLETE */
void bcp_DoBCLExpandWithOffSet2(bcp p, bcl l, bcl off)
{
  int i, j, v;
  bc c;
  int cval;
  for( i = 0; i < l->cnt; i++ )
  {
    if ( l->flags[i] == 0 )
    {
      c = bcp_GetBCLCube(p, l, i);
      for( v = 0; v < p->var_cnt; v++ )
      {
        cval = bcp_GetCubeVar(p, c, v);
        if ( cval != 3 )
        {
          bcp_SetCubeVar(p, c, v, 3);
          for( j = 0; j < off->cnt; j++ )
          {
            if ( off->flags[j] == 0 )
            {
              if ( bcp_IsIntersectionCube(p, c, bcp_GetBCLCube(p, off, j)) != 0 )
              {
                bcp_SetCubeVar(p, c, v, cval);    // there is an intersection with the off-set, so undo the don't care
                break;
              }
            }
          }
        }
      }
    }
  }
}


void bcp_DoBCLExpandWithOffSet(bcp p, bcl l, bcl off)
{
  int i, j, v;
  bc c;
  int cval;
  int is_expanded;      // set to 1 if the cube c had been successfully expanded

  bcp_StartCubeStackFrame(p);
  c = bcp_GetTempCube(p);
  
  for( i = 0; i < l->cnt; i++ ) // l->cnt may grow so new cubes are also analysed
  {
    if ( l->flags[i] == 0 )
    {
      // c = bcp_GetBCLCube(p, l, i);
      bcp_CopyCube(p, c, bcp_GetBCLCube(p, l, i));
      is_expanded = 0;
      for( v = 0; v < p->var_cnt; v++ )
      {
        cval = bcp_GetCubeVar(p, c, v);
        
        if ( cval != 3 )
        {
          bcp_SetCubeVar(p, c, v, 3);  // for testing, set the variable to don't care
          for( j = 0; j < off->cnt; j++ )       // check whether there is any intersection with the off-set 
          {
            if ( off->flags[j] == 0 )
            {
              if ( bcp_IsIntersectionCube(p, c, bcp_GetBCLCube(p, off, j)) != 0 )
              {
                break;  // intersection found
              }
            }
          } // off-set test
          if ( j >= off->cnt )  // no intersection found, so this cube was successful expanded
          {
            is_expanded = 1;
            bcp_AddBCLCubeByCube(p, l, c);
            // c = bcp_GetBCLCube(p, l, i);        // this is tricky: the address of c might have changed due to realloc of the cube list, so re-read c here
          }
          bcp_SetCubeVar(p, c, v, cval);    // undo the change for the next test
        } // dc?
        
        
      } // with all variables in c
      if ( is_expanded )
      {
        // c was expanded, so remove that cube from the list
        l->flags[i] = 1;
      }        
    } // not deleted in l
  }
  bcp_PurgeBCL(p, l);  // remove all deleted cubes from "l"
  bcp_EndCubeStackFrame(p);  
}


void bcp_DoBCLExpandWithCofactor(bcp p, bcl l)
{
  int i, v;
  bc c;
  int cval;
  int is_expanded;      // set to 1 if the cube c had been successfully expanded
  
  bcp_StartCubeStackFrame(p);
  c = bcp_GetTempCube(p);
 
  
  
  for( i = 0; i < l->cnt; i++ ) // l->cnt may grow so new cubes are also analysed
  {
    if ( l->flags[i] == 0 )
    {
      bcp_CopyCube(p, c, bcp_GetBCLCube(p, l, i));
      is_expanded = 0;
      for( v = 0; v < p->var_cnt; v++ )
      {
        cval = bcp_GetCubeVar(p, c, v);
        
        if ( cval != 3 )
        {
          bcp_SetCubeVar(p, c, v, 3);  // for testing, set the variable to don't care
          if ( bcp_IsBCLCubeCovered(p, l, c) ) // still covered, so this cube was successful expanded
          {
            is_expanded = 1;
            bcp_AddBCLCubeByCube(p, l, c);
          }
          bcp_SetCubeVar(p, c, v, cval);    // undo the change for the next test
        } // dc?
        
        
      } // with all variables in c
      if ( is_expanded )
      {
        // c was expanded, so remove that cube from the list
        l->flags[i] = 1;
      }        
    } // not deleted in l
  }
  bcp_PurgeBCL(p, l);  // remove all deleted cubes from "l"
  bcp_EndCubeStackFrame(p);  
}

