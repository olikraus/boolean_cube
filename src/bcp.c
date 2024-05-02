/*

  bcp.c
  
  boolean cube problem

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

*/

#include "bc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


int bcp_GetVarCntFromString(const char *s)
{
  int cnt = 0;
  for(;;)
  {
    while( *s == ' ' || *s == '\t' )            // skip white space
      s++;
    if ( *s == '\0' || *s == '\r' || *s == '\n' )       // stop looking at further chars if the line/string ends
    {
      return cnt;
    }
    s++;
    cnt++;
  }
  return cnt; // we should never reach this statement
}

static void bcp_var_cnt_clear(bcp p)
{
  bcp_DeleteBCL(p, p->global_cube_list);
  p->global_cube_list = NULL;
  bcp_DeleteBCL(p, p->stack_cube_list);
  p->stack_cube_list = NULL;
  free(p->cube_to_str);
  p->cube_to_str = NULL;
}


static int bcp_var_cnt_init(bcp p, size_t var_cnt)
{
  p->var_cnt = var_cnt;
  p->vars_per_blk_cnt = sizeof(__m128i)*4;
  p->blk_cnt = (var_cnt + p->vars_per_blk_cnt-1)/p->vars_per_blk_cnt;
  p->bytes_per_cube_cnt = p->blk_cnt*sizeof(__m128i);
  //printf("p->bytes_per_cube_cnt=%d\n", p->bytes_per_cube_cnt);
  p->stack_depth = 0;
  p->cube_to_str = (char *)malloc(p->var_cnt+1); 
  if ( p->cube_to_str != NULL )
  {
    p->stack_cube_list = bcp_NewBCL(p);
    if ( p->stack_cube_list != NULL )
    {
      p->global_cube_list = bcp_NewBCL(p);
      if ( p->global_cube_list != NULL )
      {
        int i;
                    /*
                            0..3:	constant cubes for all illegal, all zero, all one and all don't care
                            4..7:	uint8_t counters for zeros in a list
                            8..11:	uint8_t counters for ones in a list
                            4..11: uint16_t counters
                            12..19: uint16_t counters
                    */
        for( i = 0; i < 4+8+8; i++ )
          bcp_AddBCLCube(p, p->global_cube_list);
        if ( p->global_cube_list->cnt >= 4 )
        {
          memset(bcp_GetBCLCube(p, p->global_cube_list, 0), 0, p->bytes_per_cube_cnt);  // all vars are illegal
          memset(bcp_GetBCLCube(p, p->global_cube_list, 1), 0x55, p->bytes_per_cube_cnt);  // all vars are zero
          memset(bcp_GetBCLCube(p, p->global_cube_list, 2), 0xaa, p->bytes_per_cube_cnt);  // all vars are one
          memset(bcp_GetBCLCube(p, p->global_cube_list, 3), 0xff, p->bytes_per_cube_cnt);  // all vars are don't care
          return 1;
        }
        bcp_DeleteBCL(p, p->global_cube_list);
      }
      bcp_DeleteBCL(p, p->stack_cube_list);
    }
    free(p->cube_to_str);
  }
  return 0;
}


bcp bcp_New(size_t var_cnt)
{
  bcp p = (bcp)malloc(sizeof(struct bcp_struct));
  if ( p != NULL )
  {
      p->var_map = NULL;
      p->var_list = NULL;

      p->x_true = '1';
      p->x_false = '0';
      p->x_end = '.';
      p->x_not = '-';
      p->x_or = '|';
      p->x_and = '&';
      p->x_var_cnt = 0;
    
      if ( bcp_var_cnt_init(p, var_cnt) != 0 )
      {
        return p;
      }
      
      free(p);
  }
  return NULL;
}

/*      
  Update p->var_cnt from p->x_var_cnt after expression analysis
  Idea is this:
    1. allocate a bcp with dummy value 1
    2. parse expression and update p->x_var_cnt with "bcp_Parse()"
    3. Call this function to change p->var_cnt to p->x_var_cnt
*/
int bcp_UpdateFromBCX(bcp p)
{
  assert( p->var_cnt <= 1 );
  bcp_var_cnt_clear(p);
  if ( bcp_var_cnt_init(p, p->x_var_cnt) != 0 )
  {
    return 1;
  }
  return 0;
}

void bcp_Delete(bcp p)
{
  if ( p == NULL )
    return ;
  bcp_var_cnt_clear(p);
  
  if ( p->var_list != NULL )
    coDelete(p->var_list);
  if ( p->var_map != NULL )
    coDelete(p->var_map);  
  free(p);
}

#ifndef bcp_GetGlobalCube
bc bcp_GetGlobalCube(bcp p, int pos)
{
  assert(pos < p->global_cube_list->cnt && pos >= 0);
  return bcp_GetBCLCube(p, p->global_cube_list, pos);  
}
#endif
/*
#define bcp_GetGlobalCube(p, pos) \
  bcp_GetBCLCube((p), (p)->global_cube_list, (pos))  
*/

/* copy global cube at pos to cube provided in "r" */
void bcp_CopyGlobalCube(bcp p, bc r, int pos)
{
  bcp_CopyCube(p, r, bcp_GetGlobalCube(p, pos));
}


/* start a new stack frame for bcp_GetTempCube() */
void bcp_StartCubeStackFrame(bcp p)
{
  if ( p->stack_depth >= BCP_MAX_STACK_FRAME_DEPTH )
  {
    assert(p->stack_depth < BCP_MAX_STACK_FRAME_DEPTH);  // output error message
    exit(1); // just ensure, that we do exit, also incases if NDEBUG is active
  }
  p->stack_frame_pos[p->stack_depth] = p->stack_cube_list->cnt;
  p->stack_depth++;    
}

/* delete all cubes returned by bcp_GetTempCube() since corresponding call to bcp_StartCubeStackFrame() */
void bcp_EndCubeStackFrame(bcp p)
{
  assert(p->stack_depth > 0);
  p->stack_depth--;
  p->stack_cube_list->cnt = p->stack_frame_pos[p->stack_depth] ;  // reduce the stack list to the previous size
}

/* Return a temporary cube, which will deleted with bcp_EndCubeStackFrame(). Requires a call to bcp_StartCubeStackFrame(). */
/* this is NOT MT-SAFE, each thread requires its own bcp structure */
bc bcp_GetTempCube(bcp p)
{
  int i;
  assert(p->stack_depth > 0);
  i = bcp_AddBCLCube(p, p->stack_cube_list);
  if ( i < p->stack_frame_pos[p->stack_depth-1] )
  {
    assert( i >= p->stack_frame_pos[p->stack_depth-1] );
    exit(1);
  }
  return bcp_GetBCLCube(p, p->stack_cube_list, i);
}


