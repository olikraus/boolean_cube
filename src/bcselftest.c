/*

  bcselftest.c
  
  self and performance tests

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

*/

#include "bc.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>




bcl bcp_NewBCLWithRandomTautology(bcp p, int size, int dc2one_conversion_cnt)
{
  bcl l = bcp_NewBCL(p);
  int cube_pos = bcp_AddBCLCubeByCube(p, l, bcp_GetGlobalCube(p, 3));
  int var_pos = 0; 
  unsigned value;
  int i;

  for(;;)
  {
    cube_pos = rand() % l->cnt;
    var_pos = rand() % p->var_cnt;  
    value = bcp_GetCubeVar(p, bcp_GetBCLCube(p, l, cube_pos), var_pos);
    if ( value == 3 )
    {
      bcp_SetCubeVar(p, bcp_GetBCLCube(p, l, cube_pos), var_pos, 1);
      cube_pos = bcp_AddBCLCubeByCube(p, l, bcp_GetBCLCube(p, l, cube_pos));
      bcp_SetCubeVar(p, bcp_GetBCLCube(p, l, cube_pos), var_pos, 2);
    }
    if ( l->cnt >= size )
      break;
  }

  for( i = 0; i < dc2one_conversion_cnt; i++ )
  {
    for(;;)
    {
      cube_pos = rand() % l->cnt;
      var_pos = rand() % p->var_cnt;  
      value = bcp_GetCubeVar(p, bcp_GetBCLCube(p, l, cube_pos), var_pos);
      if ( value == 3 )
      {
        bcp_SetCubeVar(p, bcp_GetBCLCube(p, l, cube_pos), var_pos, 2);
        break;
      }
    }
  }
  
  return l;
}



/*============================================================*/



void internalTest(int var_cnt)
{
  bcp p = bcp_New(var_cnt);
  bcl t = bcp_NewBCLWithRandomTautology(p, var_cnt, 0);
  bcl r = bcp_NewBCLWithRandomTautology(p, var_cnt, var_cnt);
  bcl l = bcp_NewBCL(p);
  bcl m = bcp_NewBCL(p);
  bcl n;
  bcl nn;
  bcl nnn;
  
  int tautology;
  int equal;

  
  printf("tautology test 1\n");
  tautology = bcp_IsBCLTautology(p, t);
  assert(tautology != 0);
  
  printf("copy test\n");
  bcp_CopyBCL(p, l, t);
  assert( l->cnt == t->cnt );
  
  printf("equal test 1\n");
  equal = bcp_IsBCLEqual(p, l, t);
  assert( equal != 0 );
  
  printf("tautology test 2\n");
  tautology = bcp_IsBCLTautology(p, l);
  assert(tautology != 0);

  printf("subtract test 1\n");
  bcp_SubtractBCL(p, l, t, 1);
  assert( l->cnt == 0 );

  printf("tautology test 3\n");
  tautology = bcp_IsBCLTautology(p, r);
  assert(tautology == 0);

  printf("subtract test 2\n");
  bcp_ClearBCL(p, l);
  bcp_AddBCLCubeByCube(p, l, bcp_GetGlobalCube(p, 3));  // "l" contains the universal cube
  bcp_SubtractBCL(p, l, r, 1);             // "l" contains the negation of "r"
  printf("subtract result size %d\n", l->cnt);
  assert( l->cnt != 0 );

  printf("intersection test\n");
  bcp_IntersectionBCLs(p, m, l, r);
  printf("intersection result  m->cnt=%d l->cnt=%d r->cnt=%d\n", m->cnt, l->cnt, r->cnt);
  assert( m->cnt == 0 );


  printf("tautology test 4\n");
  bcp_AddBCLCubesByBCL(p, l, r);
  //bcp_DoBCLSingleCubeContainment(p, l);
  printf("merge result size %d\n", l->cnt);
  tautology = bcp_IsBCLTautology(p, l);
  assert(tautology != 0);               // error with var_cnt == 20 

  bcp_CopyBCL(p, l, t);
  assert( l->cnt == t->cnt );
  printf("subtract test 3\n");          // repeat the subtract test with "r", use the tautology list "t" instead of the universal cube
  bcp_SubtractBCL(p, l, r, 1);             // "l" contains the negation of "r"
  assert( l->cnt != 0 );
  printf("intersection test 2\n");
  bcp_IntersectionBCLs(p, m, l, r);
  assert( m->cnt == 0 );

  printf("tautology test 5\n");
  bcp_AddBCLCubesByBCL(p, l, r);
  printf("merge result size %d\n", l->cnt);
  //bcp_ShowBCL(p, bcp_NewBCLComplementWithCofactor(p, l));
  tautology = bcp_IsBCLTautology(p, l);
  assert(tautology != 0);               


  printf("cofactor complement test\n");
  bcp_ClearBCL(p, l);
  
  n = bcp_NewBCLComplementWithCofactor(p, r);
  printf("complement result size %d\n", n->cnt);
  assert( n->cnt != 0 );
  
  printf("simple expand\n");
  bcp_DoBCLSimpleExpand(p, n);
  //bcp_DoBCLSingleCubeContainment(p, n);
  printf("simple expand new size %d\n", n->cnt);


  printf("subtract complement test\n");
  nn = bcp_NewBCLComplementWithSubtract(p, n);  // complement of the complement, so this should be equal to r
  assert( nn->cnt != 0 );
  bcp_ShowBCL(p, nn);

  printf("equal test 2\n");
  equal = bcp_IsBCLEqual(p, r, nn);
  assert( equal != 0 );
  

  printf("intersection complement test\n");
  nnn = bcp_NewBCLComplementWithIntersection(p, n);  // complement of the complement, so this should be equal to r
  //nnn = bcp_NewBCLComplementWithSubtract(p, n);  // complement of the complement, so this should be equal to r
  assert( nnn->cnt != 0 );
  bcp_ShowBCL(p, nnn);

  printf("equal test 3\n");  
  equal = bcp_IsBCLEqual(p, r, nnn);
  assert( equal != 0 );
  


  printf("intersection test 3\n");
  bcp_IntersectionBCLs(p, m, n, r);
  printf("intersection result  m->cnt=%d n->cnt=%d r->cnt=%d\n", m->cnt, n->cnt, r->cnt);
  assert( m->cnt == 0 );

  printf("tautology test 6\n");
  bcp_AddBCLCubesByBCL(p, n, r);
  //bcp_DoBCLSingleCubeContainment(p, l);
  printf("merge result size %d\n", n->cnt);
  //bcp_ShowBCL(p, bcp_NewBCLComplementWithSubtract(p, r));
  tautology = bcp_IsBCLTautology(p, n);
  assert(tautology != 0);


  bcp_DeleteBCL(p,  nn);
  bcp_DeleteBCL(p,  n);
  bcp_DeleteBCL(p,  m);
  bcp_DeleteBCL(p,  t);
  bcp_DeleteBCL(p,  r);
  bcp_DeleteBCL(p,  l);
  bcp_Delete(p); 

}

void speedTest(int cnt) 
{
  int is_subset = 0;
  clock_t t0, t1;
  clock_t t_cofactor = 0;
  clock_t t_subtract = 0;
  clock_t t_intersection = 0;
  bcp p = bcp_New(cnt);
  bcl a = bcp_NewBCLWithRandomTautology(p, cnt+2, cnt);
  bcl b = bcp_NewBCLWithRandomTautology(p, cnt+2, cnt);
  bcl x;
  bcl ic = bcp_NewBCL(p);
  
  
  bcp_IntersectionBCLs(p, ic, a, b);
  assert( ic->list != 0 );
  printf("raw  ic->cnt = %d\n", ic->cnt);
  bcp_MinimizeBCL(p, ic);
  printf("mini ic->cnt = %d\n", ic->cnt);
  
  //puts("original:");
  //bcp_ShowBCL(p, l);

  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithSubtract(p, a, ic);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithSubtract(p, a, ic): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_subtract += t1-t0;

  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithCofactor(p, a, ic);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithCofactor(p, a, ic): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_cofactor += t1-t0;

  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithSubtract(p, ic, a);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithSubtract(p, ic, a): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_subtract += t1-t0;

  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithCofactor(p, ic, a);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithCofactor(p, ic, a): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_cofactor += t1-t0;


  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithSubtract(p, b, ic);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithSubtract(p, b, ic): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_subtract += t1-t0;

  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithCofactor(p, b, ic);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithCofactor(p, b, ic): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_cofactor += t1-t0;

  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithSubtract(p, ic, b);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithSubtract(p, ic, b): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_subtract += t1-t0;

  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithCofactor(p, ic, b);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithCofactor(p, ic, b): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_cofactor += t1-t0;


  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithSubtract(p, a, b);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithSubtract(p, a, b): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_subtract += t1-t0;

  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithCofactor(p, a, b);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithCofactor(p, a, b): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_cofactor += t1-t0;

  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithSubtract(p, b, a);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithSubtract(p, b, a): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_subtract += t1-t0;
  
  t0 = clock();
  is_subset = bcp_IsBCLSubsetWithCofactor(p, b, a);
  t1 = clock();  
  printf("bcp_IsBCLSubsetWithCofactor(p, b, a): is_subset=%d clock=%ld\n", is_subset, t1-t0);  
  t_cofactor += t1-t0;
  
  printf("subset with substract total %ld\n", t_subtract);
  printf("subset with cofactor total %ld\n", t_cofactor);
  
  if ( t_subtract < t_cofactor )
    printf("bcp_IsBCLSubsetWithSubtract is faster\n");
  else
    printf("bcp_IsBCLSubsetWithCofactor is faster\n");

  t_subtract = 0;
  t_cofactor = 0;
  t_intersection = 0;

  t0 = clock();
  x = bcp_NewBCLComplementWithSubtract(p, a);
  t1 = clock();  
  bcp_DeleteBCL(p, x);
  t_subtract += t1-t0;

  t0 = clock();
  x = bcp_NewBCLComplementWithCofactor(p, a);
  t1 = clock();  
  bcp_DeleteBCL(p, x);
  t_cofactor += t1-t0;

  t0 = clock();
  x = bcp_NewBCLComplementWithIntersection(p, a);
  t1 = clock();  
  bcp_DeleteBCL(p, x);
  t_intersection += t1-t0;


  t0 = clock();
  x = bcp_NewBCLComplementWithSubtract(p, b);
  t1 = clock();  
  bcp_DeleteBCL(p, x);
  t_subtract += t1-t0;

  t0 = clock();
  x = bcp_NewBCLComplementWithCofactor(p, b);
  t1 = clock();  
  bcp_DeleteBCL(p, x);
  t_cofactor += t1-t0;

  t0 = clock();
  x = bcp_NewBCLComplementWithIntersection(p, b);
  t1 = clock();  
  bcp_DeleteBCL(p, x);
  t_intersection += t1-t0;

  printf("complement with substract total %ld\n", t_subtract);
  printf("complement with cofactor total %ld\n", t_cofactor);
  printf("complement with intersection total %ld\n", t_intersection);
  
  /*
  if ( t_subtract < t_cofactor )
    printf("bcp_NewBCLComplementWithSubtract is faster\n");
  else
    printf("bcp_NewBCLComplementWithCofactor is faster\n");
    */

  
  bcp_DeleteBCL(p,  a);
  bcp_DeleteBCL(p,  b);
  bcp_DeleteBCL(p,  ic);

  bcp_Delete(p);  
  
}



void minimizeTest(int cnt) 
{
  clock_t t0, t1;
  bcp p = bcp_New(cnt);
  bcl a = bcp_NewBCLWithRandomTautology(p, cnt+2, cnt);
  bcl b = bcp_NewBCLWithRandomTautology(p, cnt+2, cnt);
  bcl ic = bcp_NewBCL(p);
  
  
  bcp_IntersectionBCLs(p, ic, a, b);
  assert( ic->list != 0 );
  
  
  printf("raw ic->cnt = %d\n", ic->cnt);
  t0 = clock();
  bcp_MinimizeBCL(p, ic);
  t1 = clock();  
  printf("mini ic->cnt = %d, clock = %ld\n", ic->cnt, t1-t0);
  
  printf("raw a->cnt = %d\n", a->cnt);
  t0 = clock();
  bcp_MinimizeBCL(p, a);
  t1 = clock();  
  printf("mini a->cnt = %d, clock = %ld\n", a->cnt, t1-t0);

  printf("raw b->cnt = %d\n", b->cnt);
  t0 = clock();
  bcp_MinimizeBCL(p, b);
  t1 = clock();  
  printf("mini b->cnt = %d, clock = %ld\n", b->cnt, t1-t0);


  bcp_DeleteBCL(p,  a);
  bcp_DeleteBCL(p,  b);
  bcp_DeleteBCL(p,  ic);

  bcp_Delete(p);  
  
}

static void expression_test_sub(const char *cubes, const char *expression, int is_not_propagation)
{
  bcp p;
  bcx x;
  bcl lx, lc;
  int equal;
  char *expr2;

  printf("Parse test '%s'\n", expression);
  p = bcp_New(0);
  x = bcp_Parse(p, expression, is_not_propagation);
  assert(x != NULL);
  bcp_UpdateFromBCX(p);
  
  assert( bcp_GetVarCntFromString(cubes) == p->var_cnt );  // cube size must be equal to the number of variables in the expression

  lx = bcp_NewBCLByBCX(p, x);
  bcp_DeleteBCX(p, x);          // abstract syntax tree is not required any more
  assert( lx != NULL );
  lc = bcp_NewBCLByString(p, cubes);
  assert( lc != NULL );

  printf("Equal test 1 '%s'\n", expression);
  equal = bcp_IsBCLEqual(p, lx, lc);                // checks whether the two lists are equal
  assert( equal != 0 );
  expr2 = bcp_GetExpressionBCL(p, lx);       // convert "lx" back to a human readable expression, return value must be free'd if not NULL
  assert( expr2 != NULL );
  bcp_DeleteBCL(p, lx);
  printf("Back conversion '%s' --> '%s'\n", expression, expr2);
  x = bcp_Parse(p, expr2, is_not_propagation);  // parse again
  assert( x != NULL );
  assert( p->var_cnt == p->x_var_cnt);          // the variable count must not change

  printf("Equal test 2 '%s'\n", expr2);
  free( expr2 );
  lx = bcp_NewBCLByBCX(p, x);   // again get the bcl from it
  assert( lx != NULL );
  bcp_DeleteBCX(p, x);          // abstract syntax tree is not required any more
  equal = bcp_IsBCLEqual(p, lx, lc);                // checks whether the two lists are equal
  assert( equal != 0 );

  bcp_DeleteBCL(p, lx);

  bcp_DeleteBCL(p, lc);
  bcp_Delete(p);
}

void expressionTest(void)
{
  expression_test_sub("1", "a", 1);
  expression_test_sub("11", "a&b", 1);
  expression_test_sub("00", "-(a|b)", 0);
  expression_test_sub("00", "-(a|b)", 1);
  expression_test_sub("--1\n1--\n-1-\n", "a|b|c", 1);
  expression_test_sub("--0\n1--\n-1-\n", "a|b|-c", 1);
  expression_test_sub("--11\n1--1\n-1-1\n", "(a|b|c)&d", 1);
  expression_test_sub("--11\n1--1\n-1-1\n", "-(-a&-b&-c)&d", 1);
  expression_test_sub("--11\n1--1\n-1-1\n", "-(-a&-b&-c)&d", 0);
}