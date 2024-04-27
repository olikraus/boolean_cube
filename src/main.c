/*

  main.c
  
  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

*/


#include "bc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/times.h>

/*============================================================*/


int mainx(void)
{
  bcp p = bcp_New(65);
  bc c;
  int i;
  
  printf("blk_cnt = %d\n", p->blk_cnt);
  bcp_StartCubeStackFrame(p);
  c = bcp_GetTempCube(p);
  printf("%s\n", bcp_GetStringFromCube(p, c));
  //bcp_SetCubeByString(p, c, "10-11--1----1111-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-1-0-0-0-01");
  bcp_SetCubeByString(p, c, "1111111111111111-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-1-0-0-0-01");

  printf("%s\n", bcp_GetStringFromCube(p, c));

  for( i = 0; i < p->var_cnt; i++ )
  {
    bcp_SetCubeVar(p, c, i, 3);
    printf("%s\n", bcp_GetStringFromCube(p, c));
  }
  
  bcp_EndCubeStackFrame(p);
  bcp_Delete(p);  
  
  
  return 0;
}

char *cubes_string= 
"1-1-11\n"
"110011\n"
"1-0-10\n"
"1001-0\n"
;



int mainy(void)
{
  bcp p = bcp_New(bcp_GetVarCntFromString(cubes_string));
  bcl l = bcp_NewBCL(p);
  bcl m = bcp_NewBCL(p);
  bcp_AddBCLCubesByString(p, l, cubes_string);
  bcp_AddBCLCubeByCube(p, m, bcp_GetGlobalCube(p, 3));
  bcp_SubtractBCL(p, m, l, 1);
  
  puts("original:");
  bcp_ShowBCL(p, l);
  puts("complement:");
  bcp_ShowBCL(p, m);

  bcp_IntersectionBCL(p, m, l);
  printf("intersction cube count %d\n", m->cnt);

  bcp_ClearBCL(p, m);
  bcp_AddBCLCubeByCube(p, m, bcp_GetGlobalCube(p, 3));
  bcp_SubtractBCL(p, m, l, 1);
  
  bcp_AddBCLCubesByBCL(p, m, l);        // add l to m
  printf("tautology=%d\n", bcp_IsBCLTautology(p, m));   // do a tautology test on the union

  
  bcp_DeleteBCL(p,  l);
  bcp_DeleteBCL(p,  m);
  bcp_Delete(p); 
  
  internalTest(21);
  return 0;
}



int main2(void)
{
  
  
  internalTest(19);
  //speedTest(21);
 
  minimizeTest(21);
  return 0;
}


int main3(int argc, char **argv)
{
  FILE *fp;
  if ( argc <= 1 )
  {
    printf("%s jsonfile\n", argv[0]);
    return 0;
  }
  fp = fopen(argv[1], "r");
  if ( fp == NULL )
    return perror(argv[1]), 0;
  bc_ExecuteJSON(fp);
  fclose(fp);
  return 0;
}


int main4()
{
  bcp p = bcp_New(1);
  bcx x = bcp_Parse(p, "a&b|c&b", 1);
  bcp_ShowBCX(p, x);
  
  
  //bcp_PrintBCX(p, x);
  
  coPrint(p->var_map); puts("");
  coPrint(p->var_list); puts("");
  
  bcp_DeleteBCX(p, x);
  bcp_Delete(p);
  puts("");
  return 0;
}

/*============================================================*/


int bc_ExecuteJSONFile(const char *jsonfilename)
{
  FILE *fp = fopen(jsonfilename, "r");
  if ( fp == NULL )
    return perror(jsonfilename), 0;
  bc_ExecuteJSON(fp);
  fclose(fp);
  return 1;
}

int bc_ExecuteDIMACSCNF(const char *dimacscnffilename)
{
  FILE *fp = fopen(dimacscnffilename, "r");
  bcp p;
  bcl l;
  int is_tautology;
  if ( fp == NULL )
    return perror(dimacscnffilename), 0;
  printf("DIMACS CNF read from %s\n", dimacscnffilename);
  p = bcp_NewByDIMACSCNF(fp);
  l = bcp_NewBCLByDIMACSCNF(p, fp);   // create a bcl from a DIMACS CNF
  printf("DIMACS CNF with %d clauses\n", l->cnt);
  
  is_tautology = bcp_IsBCLTautology(p, l);
  printf("DIMACS CNF tautology=%d\n", is_tautology);

  
  fclose(fp);
  return 1;
}

int bc_ExecuteParse(const char *s)
{
  bcl l;
  bcp p = bcp_New(0);
  bcx x = bcp_Parse(p, s, 1);
  char *expr;
    
  bcp_ShowBCX(p, x);
  puts("");
  
  coPrint(p->var_map); puts("");
  coPrint(p->var_list); puts("");

  bcp_UpdateFromBCX(p);

  l = bcp_NewBCLByBCX(p, x);
  assert( l != NULL );

  bcp_ShowBCL(p, l);
 
  expr = bcp_GetExpressionBCL(p, l);
  puts(expr);

  free(expr);
  bcp_DeleteBCX(p, x);
  bcp_DeleteBCL(p, l);
  bcp_Delete(p);
  puts("");
  return 1;
}



void help()
{
  puts("-test");
  puts("-json <json file>");
  puts("-dimacscnf <dimacs cnf file>");
  puts("-parse <boolean expression>");
}

int main(int argc, char **argv)
{
  struct tms start, end;
  if ( *argv == NULL )
      return 0;
  times(&start);
  argv++;    // skip program name
  for(;;)
  {
    if ( (*argv) == NULL )
      break;
    if ( strcmp(*argv, "-h") == 0 )
    {
      help();
      argv++;
      exit(1);
    }
    else if ( strcmp(*argv, "-test") == 0 )
    {
      internalTest(7);
      expressionTest();
      argv++;
    }
    else if ( strcmp(*argv, "-json") == 0 )
    {
      argv++;
      if ( (*argv) == NULL )
        return puts("JSON filename missing"), 1;
      bc_ExecuteJSONFile(*argv);
      argv++;
    }
    else if ( strcmp(*argv, "-dimacscnf") == 0 )
    {
      argv++;
      if ( (*argv) == NULL )
        return puts("DIMACS CNF filename missing"), 1;
      bc_ExecuteDIMACSCNF(*argv);
      argv++;
    }
    else if ( strcmp(*argv, "-parse") == 0 )
    {
      argv++;
      if ( (*argv) == NULL )
        return puts("expression missing"), 1;
      bc_ExecuteParse(*argv);
      argv++;
    }
    else
    {
      argv++;
      help();
      return 1;
    }
      
  }
  printf("user time: %lld\n", (long long int)(end.tms_utime-start.tms_utime));
  times(&end);
  return 0;
}


int main1(void)
{
  char *s = 
"--------------------------------------01--------------------\n"
"---------------------------------------0--------------------\n"
"-----------------------------------------0------------------\n"
"-------------------------------------------0----------------\n"
"---------------------------------------------0--------------\n"
"-----------------------------------------------0------------\n"
"-------------------------------------------------0----------\n"
"--------------------------0---------------------------------\n"
"--------------------------10--------------------------------\n"
"---------------------------0---------------------1----------\n"
"----------------------------0--------------------1----------\n"
"----------------------------01------------------------------\n"
"-----------------------------0------------------------------\n"
"-----------------------------10----------------1------------\n"
"------------------------------01----------------------------\n"
"------------------------------1-----------------------------\n"
"--------------------------------1------------1--------------\n"
"-------------------------------1-------------1--------------\n"
"---------------------------------1--------------------------\n"
"--------------------------------10--------------------------\n"
"---------------------------------01--------1----------------\n"
"----------------------------------0-------------------------\n"
"----------------------------------10------------------------\n"
"-----------------------------------0-----1------------------\n"
"------------------------------------0----1------------------\n"
"------------------------------------01----------------------\n"
"-------------------------------------0----------------------\n"
"-------------------------------------1-1--------------------\n";

  
  bcp p = bcp_New(bcp_GetVarCntFromString(s));
  bcl l = bcp_NewBCL(p);
  //bcl n;
  int r, is_tautology ;

  bcp_AddBCLCubesByString(p, l, s);
  puts("original:");
  bcp_ShowBCL(p, l);

  is_tautology = bcp_IsBCLTautology(p, l);

  //r = bcp_is_bcl_partition( p, l);
  printf("is_tautology=%d\n", is_tautology);
  bcp_DeleteBCL(p,  l);
  //bcp_DeleteBCL(p,  n);

  bcp_Delete(p);  
  return 0;
}

