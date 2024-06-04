/*

  bclcore.c
  
  boolean cube list core functions

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

bcl bcp_NewBCL(bcp p)
{
  bcl l = (bcl)malloc(sizeof(struct bcl_struct));
  if ( l != NULL )
  {
    l->cnt = 0;
    l->max = 0;
    l->last_deleted = -1;
    l->list = NULL;
    l->flags = NULL;
    return l;
  }
  return NULL;
}

bcl bcp_NewBCLByBCL(bcp p, bcl l)
{
  bcl n = bcp_NewBCL(p);
  if ( n != NULL )
  {
    n->list = (__m128i *)malloc(l->cnt*p->bytes_per_cube_cnt);
    if ( n->list != NULL )
    {
      n->flags = (uint8_t *)malloc(l->cnt*sizeof(uint8_t));
      if ( n->flags != NULL )
      {
        n->cnt = l->cnt;
        n->max = l->cnt;
        memcpy(n->list, l->list, l->cnt*p->bytes_per_cube_cnt);
        memcpy(n->flags, l->flags, l->cnt*sizeof(uint8_t));
        return n;
      }
      free(n->list);
    }
    free(n);
  }
  return NULL;
}

/*
  create a new BCL and add one cube from the global cube list into it.
  This can be used to create a tautology list:
  bcp_NewBCLWithCube(p, 3)
*/
bcl bcp_NewBCLWithCube(bcp p, int global_cube_pos)
{
  bcl l = bcp_NewBCL(p);
  if ( l != NULL )
  {
    if ( bcp_AddBCLCubeByCube(p, l, bcp_GetGlobalCube(p, global_cube_pos)) >= 0 )
    {
      return l;
    }
    bcp_DeleteBCL(p, l);
  }
  return NULL;
}


/* let a be a copy of b: copy content from bcl b into bcl a */
int bcp_CopyBCL(bcp p, bcl a, bcl b)
{
  if ( a->max < b->cnt )
  {
    __m128i *list;
    uint8_t *flags;
    if ( a->list == NULL )
      list = (__m128i *)malloc(b->cnt*p->bytes_per_cube_cnt);
    else
      list = (__m128i *)realloc(a->list, b->cnt*p->bytes_per_cube_cnt);
    if ( list == NULL )
      return 0;
    a->list = list;
    if ( a->flags == NULL )
      flags = (uint8_t *)malloc(b->cnt*sizeof(uint8_t));
    else
      flags = (uint8_t *)realloc(a->flags, b->cnt*sizeof(uint8_t));
    if ( flags == 0 )
      return 0;
    a->flags = flags;
    a->max = b->cnt;
  }
  a->cnt = b->cnt;
  memcpy(a->list, b->list, a->cnt*p->bytes_per_cube_cnt);
  memcpy(a->flags, b->flags, a->cnt*sizeof(uint8_t));
  return 1;
}

void bcp_ClearBCL(bcp p, bcl l)
{
  l->cnt = 0;
}


void bcp_DeleteBCL(bcp p, bcl l)
{
  if ( l->list != NULL )
    free(l->list);
  if ( l->flags != NULL )
    free(l->flags);
  free(l);
}

#define BCL_EXTEND 32
int bcp_ExtendBCL(bcp p, bcl l)
{
  __m128i *list;
  uint8_t *flags;
  if ( l->list == NULL )
    list = (__m128i *)malloc(BCL_EXTEND*p->bytes_per_cube_cnt);
  else
    list = (__m128i *)realloc(l->list, (BCL_EXTEND+l->max)*p->bytes_per_cube_cnt);
  if ( list == NULL )
    return 0;
  l->list = list;
  if ( l->flags == NULL )
    flags = (uint8_t *)malloc(BCL_EXTEND*sizeof(uint8_t));
  else
    flags = (uint8_t *)realloc(l->flags, (BCL_EXTEND+l->max)*sizeof(uint8_t));
  if ( flags == 0 )
    return 0;
  l->flags = flags;
  
  l->max += BCL_EXTEND;
  return 1;
}

#ifndef bcp_GetBCLCube
bc bcp_GetBCLCube(bcp p, bcl l, int pos)
{
  assert( pos < l->cnt);
  return (bc)(((uint8_t *)(l->list)) + pos * p->bytes_per_cube_cnt);
}
#endif

void bcp_ShowBCL(bcp p, bcl l)
{
  int i, list_cnt = l->cnt;
  for( i = 0; i < list_cnt; i++ )
  {
    printf("%04d %02x %s\n", i, l->flags[i], bcp_GetStringFromCube(p, bcp_GetBCLCube(p, l, i)) );
  }
}

void bcp_Show2BCL(bcp p, bcl l1, bcl l2)
{
  int i, j, list_cnt = l1->cnt;
  int v1, v2;
  bc c1, c2;
  if ( list_cnt < l2->cnt )
    list_cnt = l2->cnt;
  for( i = 0; i < list_cnt; i++ )
  {
    printf("%04d %02x %s", i, l1->flags[i], i<l1->cnt?bcp_GetStringFromCube(p, bcp_GetBCLCube(p, l1, i)):"" );
    printf(" %02x %s ", l2->flags[i], i<l2->cnt?bcp_GetStringFromCube(p, bcp_GetBCLCube(p, l2, i)):"" );
    if ( i < l1->cnt && i < l2->cnt )
    {
      c1 = bcp_GetBCLCube(p, l1, i);
      c2 = bcp_GetBCLCube(p, l2, i);
      for( j = 0; j < p->var_cnt; j++ )
      {
        v1 = bcp_GetCubeVar(p, c1, j);
        v2 = bcp_GetCubeVar(p, c2, j);
        if ( v1 == v2 )
        {
          printf(".");
        }
        else
        {
          printf("x");
        }
        
      }
    }
    printf("\n");

  }
}


int bcp_IsPurgeUsefull(bcp p, bcl l)
{
  int i = 0;
  int cnt = l->cnt;
  
  for( i = 0; i < cnt; i++ )
  {
    if ( l->flags[i] != 0 )
      return 1;
  }
  return 0;
}

/*
  remove cubes from the list, which are maked as deleted
*/
void bcp_PurgeBCL(bcp p, bcl l)
{
  int i = 0;
  int j = 0;
  int cnt = l->cnt;
  
  while( i < cnt )
  {
    if ( l->flags[i] != 0 )
      break;
    i++;
  }
  j = i;
  i++;  
  for(;;)
  {
    while( i < cnt )
    {
      if ( l->flags[i] == 0 )
        break;
      i++;
    }
    if ( i >= cnt )
      break;
    memcpy((void *)bcp_GetBCLCube(p, l, j), (void *)bcp_GetBCLCube(p, l, i), p->bytes_per_cube_cnt);
    j++;
    i++;
  }
  l->cnt = j;  
  memset(l->flags, 0, l->cnt);
}


/* add a DC cube to the cube list and return the position of the new cube */
int bcp_AddBCLCube(bcp p, bcl l)
{
  //printf("bcp_AddBCLCube cnt = %d\n", l->cnt);
  /* ignore last_deleted for now */
  while ( l->max <= l->cnt )
    if ( bcp_ExtendBCL(p, l) == 0 )
      return -1;
  assert( l->list != NULL );
  assert( l->max > l->cnt );
  l->cnt++;
  bcp_ClrCube(p, bcp_GetBCLCube(p, l, l->cnt-1));
  l->flags[l->cnt-1] = 0;
  return l->cnt-1;
}

/* add a cube and return its position */
int bcp_AddBCLCubeByCube(bcp p, bcl l, bc c)
{
  while ( l->max <= l->cnt )
    if ( bcp_ExtendBCL(p, l) == 0 )
      return -1;
  assert( l->list != NULL );
  assert( l->max > l->cnt );
  l->cnt++;
  bcp_CopyCube(p, bcp_GetBCLCube(p, l, l->cnt-1), c);  
  l->flags[l->cnt-1] = 0;
  return l->cnt-1;
}


/*
  Adds the cubes from b to list a.
  Technically this is the union of a and b, which i stored in a
  This procedure does not do any simplification

  Note: 
    Maybe we need a bcp_UnionBCL() which also does in place SCC 
*/
int bcp_AddBCLCubesByBCL(bcp p, bcl a, bcl b)
{
  int i;
  for ( i = 0; i < b->cnt; i++ )
  {
    if ( b->flags[i] == 0 )
      if ( bcp_AddBCLCubeByCube(p, a, bcp_GetBCLCube(p, b, i)) < 0 )
        return 0;
  }
  return 1;
}

/*
  add cubes from the given string.
  multiple strings are added if separated by newline 
*/
int bcp_AddBCLCubesByString(bcp p, bcl l, const char *s)
{
  int cube_pos;
  for(;;)
  {
    for(;;)
    {
      if ( *s == '\0' )
        return 1;
      if ( *s > ' ' )
        break;
      s++;
    }
    cube_pos = bcp_AddBCLCube(p, l);
    if ( cube_pos < 0 )
      break;
    bcp_SetCubeByStringPointer(p, bcp_GetBCLCube(p, l, cube_pos),  &s);
  }
  return 0;     // memory error
}

bcl bcp_NewBCLByString(bcp p, const char *s)
{
  bcl l = bcp_NewBCL(p);
  if ( l == NULL )
    return NULL;
  if ( bcp_AddBCLCubesByString(p, l, s) == 0 )
    return bcp_DeleteBCL(p, l), NULL;
  return l;
}



/*
  return a list with the variable count for each cube in the list.
  this information can be used to optimize sub-set related checks

  returns an allocated array, which must be free'd by the calling function

  used by 
    void bcp_DoBCLSingleCubeContainment(bcp p, bcl l)
  to reduce the total number of checks

  used by
    void bcp_DoBCLMultiCubeContainment(bcp p, bcl l)
  to guide the check
  
  calls: bcp_GetCubeVariableCount

*/
int *bcp_GetBCLVarCntList(bcp p, bcl l)
{
  int i;
  int *vcl = (int *)malloc(sizeof(int)*l->cnt);  
  assert(l != NULL);
  if ( vcl == NULL )
    return NULL;
  for( i = 0; i < l->cnt; i++ )
  {
    if ( l->flags[i] == 0 )
    {
      vcl[i] = bcp_GetCubeVariableCount(p, bcp_GetBCLCube(p,l,i));
    }
    else
    {
      vcl[i] = -1;
    }
  }
  return vcl;
}


/*
  For each column which contains only DC, replace this DC with Zero:

  Example:
    1-10
    --11
    0---

  Becomes:
    1010
    -011
    00--

  --> second column is only DC for all terms and is replaced with zero

*/
void bcp_SetBCLAllDCToZero(bcp p, bcl l)
{
  int i, j;
  bc c;
  bc illegal_cube = bcp_GetBCLCube(p, p->global_cube_list, 0);
  
  //__m128i z = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 1));
  __m128i o = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 2));
  __m128i dc = _mm_loadu_si128(bcp_GetBCLCube(p, p->global_cube_list, 3));
  __m128i mask;

  /* loop over all blocks of the cube */  

  for( j = 0; j < p->blk_cnt; j++ )
  {
    /* initially fill the mask with all DC */
    
    mask = dc;

    /* find columns where we only have dc (bitpattern 11) in one column */
    
    for( i = 0; i < l->cnt; i++ )
    {
      c = bcp_GetBCLCube(p,l,i);
      mask = _mm_and_si128(mask, _mm_loadu_si128(c+j));
    }

    /* mask contains 11 bit pattern if there is only dc in that column, now make a proper mask out of it */
  
    //mask = _mm_and_si128( mask, _mm_srai_epi16(mask,1));          // r = r & (r >> 1)     this will generate x1 for DC values
    //mask = _mm_and_si128(mask, z);                                    // r = r & 01   this will generate 01 for DC and 00 for "10" and "01" (and also for "00")

    mask = _mm_and_si128( mask, _mm_slli_epi16(mask,1));          // r = r & (r << 1)     this will generate 1x for DC values
    mask = _mm_and_si128(mask, o);                                    // r = r & 10   this will generate 10 for DC and 00 for "10" and "01" (and also for "00")
    mask = _mm_andnot_si128(mask, dc);                                // invert mask, so we have 01 for DC and 11 for all other values, the "dc" 2nd arg is just a dummy value for andnot

    
    /* for a dc column, the mask contains the bit pattern 01 (for zero), apply this to the DC columns */
    
    for( i = 0; i < l->cnt; i++ )
    {
      
      c = bcp_GetBCLCube(p,l,i);
      // convert the dc's (11) to zero (01), but ensure, that the unused variables stay at dc by or-ing the illegal cube 
      // _mm_storeu_si128(c+j, _mm_or_si128(_mm_and_si128(mask, _mm_loadu_si128(c+j)), _mm_loadu_si128(illegal_cube+j)) );
      _mm_storeu_si128(c+j, _mm_and_si128(mask, _mm_loadu_si128(c+j)) );
     }
  } 
  bcp_ShowBCL(p, l);
}


