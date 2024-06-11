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
//#include <sys/times.h>
#include <time.h>

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
  int is_tautology ;

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
  bc_ExecuteJSON(fp, stdout, 0);
  fclose(fp);
  return 0;
}


int main4()
{
  bcp p = bcp_New(1);
  bcx x = bcp_Parse(p, "a&b|c&b", 1, 1);
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


int bc_ExecuteJSONFile(const char *json_input_filename, const char *json_output_filename, int isCompact)
{
  FILE *in_fp = fopen(json_input_filename, "r");
  FILE *out_fp = stdout;
  if ( in_fp == NULL )
    return perror(json_input_filename), 0;
  
  if ( json_output_filename != NULL )
  {
    out_fp = fopen(json_output_filename, "w");
    if ( out_fp == NULL )
      return fclose(in_fp), perror(json_output_filename), 0;
  }
  
  bc_ExecuteJSON(in_fp, out_fp, isCompact);
  fclose(in_fp);
  if ( out_fp != stdout )
    fclose(out_fp);
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
  bcx x = bcp_Parse(p, s, 1, 1);
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


const char *json_input_spec = 
"JSON Input    := [ <block> ]\n"
"block         := <bcl2slot> |  <show> | <minimize> | <intersection0> | <subtract0> | <equal0> | <exchange0> | <copy0> | <setup>\n"
"bcl2slot      := { \"cmd\":\"bcl2slot\", <bx>, \"slot\":<slnr> } Copy the given input <bx> into a slot with number <slnr>\n"
"show          := { \"cmd\":\"show\", <bxs>  }                  Show the content of <bxs>\n"
"minimize      := { \"cmd\":\"minimize\", \"slot\":<slnr>  }      Minimize the content of the specified slot\n"
"complement    := { \"cmd\":\"complement\", \"slot\":<slnr>  }    Complement the content of the specified slot\n"
"flip          := { \"cmd\":\"flip\", \"slot\":<slnr>  }          one&zero will become 'not used', 'not used' will be zero\n"
"unused2zero   := { \"cmd\":\"unused2zero\", \"slot\":<slnr>  }   Force don't care variables to 'not used' (zero) (obsolete)\n"
"unused2zero0  := { \"cmd\":\"unused2zero0\", <bxs>, <l>  }     Force don't care variables to 'not used' (zero) in slot 0, consider mask in <bxs> (obsolete)\n"
"group2zero0   := { \"cmd\":\"group2zero0\", <bxs>, <l>  }      Force don't care variables to 'not used' (zero) in slot 0, consider group vars in <bxs>\n"
"intersection0 := { \"cmd\":\"intersection0\", <bxs>, <l> }     Intersection ('AND') of slot 0 and <bxs>, result in slot 0.\n"
"union0        := { \"cmd\":\"union0\", <bxs>, <l> }            Union ('OR') of slot 0 and <bxs>, result in slot 0.\n"
"subtract0     := { \"cmd\":\"subtract0\", <bxs>, <l> }         Subtract <bxs> from slot 0, result in slot 0\n"
"equal0        := { \"cmd\":\"equal0\", <bxs>, <l>] }           Compare slot 0 with <bxs>, output result under the given label\n"
"exchange0     := { \"cmd\":\"exchange0\", \"slot\":<slnr>] }     Exchange slot 0 with slot <slnr>\n"
"copy0         := { \"cmd\":\"copy0\", \"slot\":<slnr>] }         Copy slot 0 to slot <slnr>\n"
"copy0to       := { \"cmd\":\"copy0to\", \"slot\":<slnr>] }       Copy slot 0 to slot <slnr>\n"
"copy0from     := { \"cmd\":\"copy0from\", <bxs>] }             Copy <bxs> to slot 0\n""setup         := { \"xend\":\";\", \"xand\":\"&\", \"xor\":\"|\", \"xnot\":\"-\", \"xtrue\":\"1\", \"xfalse\":\"0\" }  Redefine parser\n"  
"l             := \"label\":<key> | \"label0\":<key>            Output result flags (and slot 0 content) to the output JSON map\n"
"iv            := \"ignoreVars\":<?>                          If present, then the variables of any \"expr\" of the current cmd block are ignored\n"   
"key           := Any ASCII String                          Use this <key> as a key of the generated output JSON map\n" 
"bx            := \"bcl\":<bclsv> | \"expr\":<exprstr> | \"mtvar\":<mtstr> | <iv>\n"
"bxs           := \"bcl\":<bclsv> | \"expr\":<exprstr> | \"mtvar\":<mtstr> | \"slot\":<slnr> | <iv>\n"
"bclsv         := <bclstr> | <bclvec>\n"
"bclstr        := <cubestr> <cr> <cubestr> <cr> ... <cubestr>\n"
"bclvec        := [ <cubestr> ]\n"
"cubestr       := String which contains only '0', '1', 'x' and '-'\n"
"slnr          := Integer number between 0 and " SLOT_CNT_STR "\n"
;


const char *json_output_spec = 
"JSON Output := { \"key\":<rblk> }         The keys are taken from label/label0 values of the input JSON\n"
"rblk        := { <result> }\n"
"result      := <index> | <empty> | <subset> | <superset> | <bcl> | <expr>\n"
"index       := \"index\":<integer>        The position of corresponding block in the JSON input\n"
"empty       := \"empty\":<integer>        1 if slot 0 is empty\n"
"subset      := \"subset\":<integer>       1 if slot 0 is subset of/equal with <bxs> for <equal0> cmd\n"
"superset    := \"superset\":<integer>     1 if slot 0 is superset of/equal <bxs> for <equal0> cmd\n"
"bcl         := \"bcl\":<bclvec>           Content of slot 0 as a binary cube list\n"
"expr        := \"expr\":<expr>            Content of slot 0 as a binary expression\n"
"The JSON output contains a special <rblk> with the variable definition:\n"
"\"\":{ \"vmap\":<map with variables>, \"vlist\":<vector with variables> }\n"
;



void help()
{
  puts("Boolean Cube Calculator, " __DATE__);
  puts("-h                              Print this help.");
  puts("-v                              Increase log level. Use multiple '-v' for more details");
  puts("-test                           Execute internal test procedure. Requires debug version of this executable.");
  puts("-speed                          Execute speed test procedure.");
  puts("-dimacscnf <dimacs cnf file>    SAT solver for the given DIMACS file.");
  puts("-parse <boolean expression>     Parse a given boolean expression.");
  puts("-ojpp                           Pretty print JSON output for the next '-json' command.");
  puts("-ojson <json file>              Provide filename for the JSON output of the next '-json' command.");
  puts("-json <json file>               Parse and execute commands from JSON input file.");
  printf("%s", json_input_spec);
  printf("%s", json_output_spec);
}

char *json_output_filename = NULL;
int isCompactJSONOutput = 1;

int main(int argc, char **argv)
{
  //struct tms start, end;
  if ( *argv == NULL )
      return 0;
  //times(&start);
  argv++;    // skip program name
  if ( (*argv) == NULL )
  {
    help();
    return 1;
  }
      
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
    else if ( strcmp(*argv, "-speed") == 0 )
    {
      speedTest(17);
      argv++;
    }
    else if ( strcmp(*argv, "-v") == 0 )
    {
      bc_log_level++;
      argv++;
    }
    

    
    else if ( strcmp(*argv, "-ojpp") == 0 )
    {
      isCompactJSONOutput = 0;
      argv++;
    }
    else if ( strcmp(*argv, "-ojson") == 0 )
    {
      argv++;
      if ( (*argv) == NULL )
        return puts("ojson filename missing"), 1;
      json_output_filename = *argv;
      argv++;
    }
    else if ( strcmp(*argv, "-json") == 0 )
    {
      argv++;
      if ( (*argv) == NULL )
        return puts("JSON filename missing"), 1;
      bc_ExecuteJSONFile(*argv, json_output_filename, isCompactJSONOutput);
      //times(&end);
      //printf("\nuser time: %lld\n", (long long int)(end.tms_utime-start.tms_utime));
      argv++;
    }
    else if ( strcmp(*argv, "-dimacscnf") == 0 )
    {
      argv++;
      if ( (*argv) == NULL )
        return puts("DIMACS CNF filename missing"), 1;
      bc_ExecuteDIMACSCNF(*argv);
      //times(&end);
      //printf("\nuser time: %lld\n", (long long int)(end.tms_utime-start.tms_utime));
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
      help();
      return 1;
    }
      
  }
  //times(&end);
  return 0;
}


