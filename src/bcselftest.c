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
#include <stdlib.h>
#include <string.h>
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
  assert( nnn != NULL );
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


  bcp_DeleteBCL(p,  nnn);
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
  p = bcp_New(0);                                                               // create a new (incomplete) problem structure
  x = bcp_Parse(p, expression, is_not_propagation, 1);          // parse a string expression, this will also update p>x_var_cnt
  assert(x != NULL);
  bcp_UpdateFromBCX(p);                                                      // takeover the variables from the expression into the problem structure
  
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
  x = bcp_Parse(p, expr2, is_not_propagation, 1);  // parse again
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

void expressionTest(void)   // called from main.c if command line option -test is provided
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


void exclude_test_sub(const char *expression, const char *group, const char *expected_result, int use_vars_from_group)
{
  bcp p;
  bcx x;
  bcx g;
  bcl lexpr, lgrp, lexpected;
  int equal;
  char *expr2;

  printf("Exclude test expr='%s' grp='%s'\n", expression, group);
  p = bcp_New(0);                                                               // create a new (incomplete) problem structure
  
  if ( use_vars_from_group )
  {
    g = bcp_Parse(p, group, 0, 1);          // parse a string expression, this will also update p>x_var_cnt
    assert(g != NULL);
    bcp_DeleteBCX(p, g);
  }
  
  
  x = bcp_Parse(p, expression, 1, 1);          // parse a string expression, this will also update p>x_var_cnt
  assert(x != NULL);
  bcp_UpdateFromBCX(p);                                                      // takeover the variables from the expression into the problem structure
  lexpr = bcp_NewBCLByBCX(p, x);   
  assert( lexpr != NULL );
  bcp_DeleteBCX(p, x);          // abstract syntax tree is not required any more

  x = bcp_Parse(p, group, 0, 0);
  lgrp = bcp_NewBCLByBCX(p, x);   
  assert( lgrp != NULL );
  assert( lgrp->cnt == 1 );
  bcp_DeleteBCX(p, x); 

  x = bcp_Parse(p, expected_result, 0, 0);
  lexpected = bcp_NewBCLByBCX(p, x);   
  assert( lexpected != NULL );
  bcp_DeleteBCX(p, x); 
  
  coPrint(p->var_map); puts("");
  puts("grp:");
  bcp_ShowBCL(p, lgrp);

  
  if ( bcp_DoBCLExcludeGroup(p, lexpr, bcp_GetBCLCube(p, lgrp, 0)) == 0 )
  {
    printf("Exclude test memory error\n");
    return;
  }
  
  expr2 = bcp_GetExpressionBCL(p, lexpr);       // convert "l" to a human readable expression, return value must be free'd if not NULL
  assert( expr2 != NULL );
  printf("Exclude test result='%s'\n", expr2);
  free(expr2);
  expr2 = bcp_GetExpressionBCL(p, lexpected);
  assert( expr2 != NULL );
  printf("Exclude test expected='%s'\n", expr2);
  free(expr2);
  
  
  equal = bcp_IsBCLEqual(p, lexpr, lexpected);                // checks whether the two lists are equal
  assert( equal != 0 );

  bcp_DeleteBCL(p, lexpected);
  bcp_DeleteBCL(p, lgrp);
  bcp_DeleteBCL(p, lexpr);
  bcp_Delete(p);  
}


void excludeTest()
{
  /* void exclude_test_sub(const char *expression, const char *group, const char *expected_result) */
  exclude_test_sub("a", "a", "a", 0);
  exclude_test_sub("a&b|c", "a&b", "c", 0);        // expectation is, that a&b is removed
  exclude_test_sub("a|b", "a&b", "(a&-b)|(-a&b)", 0);         // the original term needs to be updated with the negated other members
  exclude_test_sub("(a&c)|(-b&d)", "a&b", "(a&-b&c)|(a&-b&d)", 0);         // the -b term will be extended with the a term
  
  exclude_test_sub("a", "a&b&c", "a&-b&-c", 1);         // the original term needs to be updated with the negated other members
  exclude_test_sub("a", "a&b&c", "a&-b&-c", 0);         // the original term needs to be updated with the negated other members
  exclude_test_sub("a", "a&b&c", "a", 0);         // the original term needs to be updated with the negated other members
  
  exclude_test_sub("-b", "a&b&c", "a&-b&-c|-a&-b&c", 1);         // the original term needs to be updated with the negated other members

  exclude_test_sub("x", "a0&a1&a2&a3&a4&a5&a6&a7&a8&a9&b0&b1&b2&b3&b4&b5&b6&b7&b8&b9&c0&c1&c2&c3&c4&c5&c6&c7&c8&c9&d0&d1&d2&d3&d4&d5&d6&d7&d8&d9  &x", "-a0&-a1&-a2&-a3&-a4&-a5&-a6&-a7&-a8&-a9&-b0&-b1&-b2&-b3&-b4&-b5&-b6&-b7&-b8&-b9&-c0&-c1&-c2&-c3&-c4&-c5&-c6&-c7&-c8&-c9&-d0&-d1&-d2&-d3&-d4&-d5&-d6&-d7&-d8&-d9&x", 1);         // the original term needs to be updated with the negated other 
}


static void generated_expect_equal_cubes(bcp p, const char *test_name, bcl actual, const char *expected_cubes)
{
  bcl expected = bcp_NewBCLByString(p, expected_cubes);
  int equal;

  assert(expected != NULL);
  equal = bcp_IsBCLEqual(p, actual, expected);

  if ( equal == 0 )
  {
    printf("Generated test '%s' failed\n", test_name);
    printf("Expected cubes:\n%s", expected_cubes);
    printf("Expected BCL:\n");
    bcp_ShowBCL(p, expected);
    printf("Actual cubes:\n");
    bcp_ShowBCL(p, actual);
  }
  assert(equal != 0);
  bcp_DeleteBCL(p, expected);
}


static void generated_expect_cube_string(bcp p, const char *test_name, bc cube, const char *expected_cube)
{
  const char *actual_cube = bcp_GetStringFromCube(p, cube);
  if ( strcmp(actual_cube, expected_cube) != 0 )
  {
    printf("Generated test '%s' failed\n", test_name);
    printf("Expected cube '%s', got '%s'\n", expected_cube, actual_cube);
  }
  assert(strcmp(actual_cube, expected_cube) == 0);
}


void generated_test_cases(void)
{
  bcp p;
  bcl a;
  bcl b;
  bcl c;
  bcl d;
  bcl off;
  bcl grp_list;
  bc cube;
  int *vcl;
  int pos;
  int tautology;

  printf("Generated test cases\n");

  p = bcp_New(2);
  assert(p != NULL);
  printf("Generated core BCL tests\n");
  a = bcp_NewBCLWithCube(p, 3);
  assert(a != NULL);
  generated_expect_equal_cubes(p, "bcp_NewBCLWithCube", a, "--\n");

  b = bcp_NewBCLByBCL(p, a);
  assert(b != NULL);
  generated_expect_equal_cubes(p, "bcp_NewBCLByBCL", b, "--\n");

  c = bcp_NewBCL(p);
  assert(c != NULL);
  assert(bcp_CopyBCL(p, c, b) != 0);
  generated_expect_equal_cubes(p, "bcp_CopyBCL", c, "--\n");
  bcp_ClearBCL(p, c);
  assert(c->cnt == 0);

  /* Build three cubes: "10" has two assigned literals, "--" has none, "01" has two assigned literals. */
  pos = bcp_AddBCLCube(p, c);
  assert(pos == 0);
  bcp_SetCubeByString(p, bcp_GetBCLCube(p, c, 0), "10");
  pos = bcp_AddBCLCubeByCube(p, c, bcp_GetGlobalCube(p, 3));
  assert(pos == 1);
  assert(bcp_AddBCLCubesByString(p, c, "01\n") != 0);
  assert(c->cnt == 3);

  /* bcp_GetBCLVarCntList counts non-DC variables (both '0' and '1' literals). */
  /* So "10" -> 2, "--" -> 0, and "01" -> 2. */
  vcl = bcp_GetBCLVarCntList(p, c);
  assert(vcl != NULL);
  assert(vcl[0] == 2);
  assert(vcl[1] == 0);
  assert(vcl[2] == 2);
  free(vcl);

  assert(bcp_AddBCLCubesByBCL(p, c, a) != 0);
  assert(c->cnt == 4);
  c->flags[0] = 1;
  assert(bcp_IsPurgeUsefull(p, c) != 0);
  bcp_PurgeBCL(p, c);
  assert(c->cnt == 3);
  generated_expect_equal_cubes(p, "bcp_PurgeBCL", c, "--\n01\n");

  bcp_DeleteBCL(p, c);
  bcp_DeleteBCL(p, b);
  bcp_DeleteBCL(p, a);
  bcp_Delete(p);

  p = bcp_New(2);
  assert(p != NULL);
  printf("Generated cube transformation tests\n");

  /* Regression check: "-- | 0-" must be a tautology. */
  a = bcp_NewBCLByString(p, "--\n0-\n");
  assert(a != NULL);
  tautology = bcp_IsBCLTautology(p, a);
  assert(tautology == 1);
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "1-\n-0\n");
  assert(a != NULL);

  /*
    This is not a logical NOT operation.
    bcp_SetBCLFlipVariables maps: 1/0 -> -, and - -> 0.
    So 1- and -0 become -0 and 0-.
  */
  bcp_SetBCLFlipVariables(p, a);
  generated_expect_equal_cubes(p, "bcp_SetBCLFlipVariables", a, "-0\n0-\n");
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "1-\n0-\n");
  assert(a != NULL);
  bcp_SetBCLAllDCToZero(p, a, NULL);
  generated_expect_equal_cubes(p, "bcp_SetBCLAllDCToZero", a, "10\n00\n");
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "1-\n-1\n");
  assert(a != NULL);
  bcp_StartCubeStackFrame(p);
  cube = bcp_GetTempCube(p);
  assert(cube != NULL);
  bcp_AndElementsBCL(p, a, cube);
  generated_expect_cube_string(p, "bcp_AndElementsBCL", cube, "11");
  bcp_EndCubeStackFrame(p);
  bcp_AndBCL(p, a);
  generated_expect_equal_cubes(p, "bcp_AndBCL", a, "11\n");
  bcp_DeleteBCL(p, a);
  bcp_Delete(p);

  p = bcp_New(3);
  assert(p != NULL);
  printf("Generated cofactor and variable analysis tests\n");
  a = bcp_NewBCLByString(p, "1--\n0-1\n");
  assert(a != NULL);
  bcp_CalcBCLBinateSplitVariableTable(p, a);
  assert(bcp_GetBCLMaxBinateSplitVariableSimple(p, a) == 0);
  assert(bcp_GetBCLMaxBinateSplitVariable(p, a) == 0);
  assert(bcp_IsBCLVariableDC(p, a, 1) != 0);
  assert(bcp_IsBCLVariableUnate(p, a, 2, 2) != 0);
  assert(bcp_IsBCLVariableUnate(p, a, 0, 2) == 0);

  b = bcp_NewBCLByString(p, "110\n111\n");
  assert(b != NULL);
  /* bcp_DoBCLOneVariableCofactor(p, b, 0, 2):
     p = problem structure
     b = boolean cube list to calculate cofactor of
     0 = variable position (var_pos)
     2 = value (1=zero, 2=one) */
  bcp_DoBCLOneVariableCofactor(p, b, 0, 2);
  generated_expect_equal_cubes(p, "bcp_DoBCLOneVariableCofactor", b, "11-\n");
  bcp_DeleteBCL(p, b);

  b = bcp_NewBCLByString(p, "110\n111\n");
  assert(b != NULL);
  /* bcp_DoBCLOneVariableCofactor(p, b, 0, 1):
     p = problem structure
     b = boolean cube list to calculate cofactor of
     0 = variable position (var_pos)
     1 = value (1=zero, 2=one) - cofactor with variable = 0 */
  bcp_DoBCLOneVariableCofactor(p, b, 0, 1);
  generated_expect_equal_cubes(p, "bcp_DoBCLOneVariableCofactor value=1", b, "-1-\n");
  bcp_DeleteBCL(p, b);

  b = bcp_NewBCLByString(p, "110\n111\n");
  assert(b != NULL);
  /* bcp_NewBCLCofacterByVariable(p, b, 1, 2):
     p = problem structure
     b = boolean cube list to calculate cofactor of
     1 = variable position (var_pos)
     2 = value (1=zero, 2=one) - note: creates a new BCL, does not modify original */
  c = bcp_NewBCLCofacterByVariable(p, b, 1, 2);
  assert(c != NULL);
  generated_expect_equal_cubes(p, "bcp_NewBCLCofacterByVariable", c, "11-\n");
  generated_expect_equal_cubes(p, "bcp_NewBCLCofacterByVariable original", b, "110\n111\n");

  bcp_StartCubeStackFrame(p);
  cube = bcp_GetTempCube(p);
  assert(cube != NULL);
  bcp_SetCubeByString(p, cube, "1--");
  d = bcp_NewBCLByString(p, "110\n111\n011\n");
  assert(d != NULL);
  bcp_DoBCLCofactorByCube(p, d, cube, -1);
  generated_expect_equal_cubes(p, "bcp_DoBCLCofactorByCube", d, "-1-\n011\n");
  bcp_DeleteBCL(p, d);

  d = bcp_NewBCLByString(p, "110\n111\n011\n");
  assert(d != NULL);
  off = bcp_NewBCLCofactorByCube(p, d, cube, -1);
  assert(off != NULL);
  generated_expect_equal_cubes(p, "bcp_NewBCLCofactorByCube", off, "-1-\n011\n");
  bcp_DeleteBCL(p, off);
  bcp_EndCubeStackFrame(p);

  bcp_DeleteBCL(p, d);
  bcp_DeleteBCL(p, c);
  bcp_DeleteBCL(p, b);
  bcp_DeleteBCL(p, a);
  bcp_Delete(p);

  p = bcp_New(2);
  assert(p != NULL);
  printf("Generated containment tests\n");
  a = bcp_NewBCLByString(p, "1-\n11\n");
  assert(a != NULL);
  assert(bcp_IsBCLCubeSingleCovered(p, a, bcp_GetBCLCube(p, a, 1)) != 0);
  assert(bcp_IsBCLCubeCovered(p, a, bcp_GetBCLCube(p, a, 1)) != 0);
  assert(bcp_IsBCLCubeRedundant(p, a, 1) != 0);
  bcp_DoBCLSingleCubeContainment(p, a);
  generated_expect_equal_cubes(p, "bcp_DoBCLSingleCubeContainment", a, "1-\n");
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "1-\n11\n10\n");
  assert(a != NULL);
  bcp_DoBCLMultiCubeContainment(p, a);
  generated_expect_equal_cubes(p, "bcp_DoBCLMultiCubeContainment", a, "11\n10\n");
  bcp_DeleteBCL(p, a);
  bcp_Delete(p);

  p = bcp_New(2);
  assert(p != NULL);
  printf("Generated set operation tests\n");
  a = bcp_NewBCLByString(p, "1-\n-1\n");
  assert(a != NULL);
  b = bcp_NewBCLComplement(p, a);
  assert(b != NULL);
  generated_expect_equal_cubes(p, "bcp_NewBCLComplement", b, "00\n");
  bcp_DeleteBCL(p, b);

  assert(bcp_ComplementBCL(p, a) != 0);
  generated_expect_equal_cubes(p, "bcp_ComplementBCL", a, "00\n");
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "1-\n");
  b = bcp_NewBCLByString(p, "11\n");
  assert(a != NULL);
  assert(b != NULL);
  assert(bcp_IsBCLSubsetWithCofactor(p, a, b) != 0);
  assert(bcp_IsBCLSubsetWithSubtract(p, a, b) != 0);
  assert(bcp_IsBCLSubset(p, a, b) != 0);
  assert(bcp_IsBCLSubset(p, b, a) == 0);
  bcp_DeleteBCL(p, b);
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "1-\n-1\n");
  b = bcp_NewBCLByString(p, "1-\n-0\n");
  assert(a != NULL);
  assert(b != NULL);
  assert(bcp_IntersectionBCL(p, a, b) != 0);
  generated_expect_equal_cubes(p, "bcp_IntersectionBCL", a, "1-\n");
  bcp_DeleteBCL(p, b);
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "11\n10\n");
  assert(a != NULL);
  bcp_DoBCLSimpleExpand(p, a);
  generated_expect_equal_cubes(p, "bcp_DoBCLSimpleExpand", a, "1-\n");
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "11\n10\n");
  off = bcp_NewBCLByString(p, "0-\n");
  assert(a != NULL);
  assert(off != NULL);
  bcp_DoBCLExpandWithOffSet(p, a, off);
  generated_expect_equal_cubes(p, "bcp_DoBCLExpandWithOffSet", a, "1-\n");
  bcp_DeleteBCL(p, off);
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "11\n10\n");
  assert(a != NULL);
  bcp_DoBCLExpandWithCofactor(p, a);
  generated_expect_equal_cubes(p, "bcp_DoBCLExpandWithCofactor", a, "1-\n");
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "11\n10\n");
  assert(a != NULL);
  bcp_MinimizeBCLWithOnSet(p, a);
  generated_expect_equal_cubes(p, "bcp_MinimizeBCLWithOnSet", a, "1-\n");
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "11\n10\n");
  assert(a != NULL);
  bcp_MinimizeBCL(p, a);
  generated_expect_equal_cubes(p, "bcp_MinimizeBCL", a, "1-\n");
  bcp_DeleteBCL(p, a);
  bcp_Delete(p);

  p = bcp_New(2);
  assert(p != NULL);
  printf("Generated exclude-group tests\n");
  a = bcp_NewBCLByString(p, "1-\n-1\n");
  grp_list = bcp_NewBCLByString(p, "11\n");
  assert(a != NULL);
  assert(grp_list != NULL);
  assert(bcp_DoBCLExcludeGroupList(p, a, grp_list) != 0);
  generated_expect_equal_cubes(p, "bcp_DoBCLExcludeGroupList", a, "10\n01\n");
  bcp_DeleteBCL(p, a);

  a = bcp_NewBCLByString(p, "1-\n-1\n");
  p->exclude_group_list = grp_list;
  assert(bcp_DoBCLXGroup(p, a) != 0);
  generated_expect_equal_cubes(p, "bcp_DoBCLXGroup", a, "10\n01\n");
  p->exclude_group_list = NULL;
  bcp_DeleteBCL(p, grp_list);
  bcp_DeleteBCL(p, a);
  bcp_Delete(p);
}