/*

  bc.h
  
  boolean cube
  
  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/
  

  Suggested article: R. Rudell "Multiple-Valued Logic Minimization for PLA Synthesis"
    https://www2.eecs.berkeley.edu/Pubs/TechRpts/1986/734.html
  This code will only use the binary case (and not the multiple value case) which is partly described in the above article.  

  https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#ssetechs=SSE,SSE2&ig_expand=80

  gcc -g -Wall -fsanitize=address bc.c
  
  predecessor output
  echo | gcc -dM -E -  
  echo | gcc -march=native -dM -E -
  echo | gcc -march=silvermont -dM -E -         // Pentium N3710: sse4.2 popcnt 
 

  
  Definitions
  
  - Variable: A boolean variable which is one (1), zero (0), don't care (-) or illegal (x), A variable occupies 2 bit.
  - Variable encoding: A variable is encoded with two bits: one=10, zero=01, don't care=11, illegal=00
  - Blk (block): A physical unit in the uC which can group multiple variables, currently this is a __m128i object
  - Boolean Cube (BC): A vector of multiple blocks, which can hold all variables of a boolean cube problem. Usually this is interpreted as a "product" ("and" operation) of the boolean variables
  - Boolean Cube Problem (BCP): A master structure with a given number of boolean variables ("var_cnt") 
  - Boolean Cube List (BCP): A list of Boolean Cubes
  - Boolean Cube Expression (BCX): A node inside an abstract syntax tree for a boolean expression.

*/

#ifndef BC_H
#define BC_H

#include <x86intrin.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "co.h"


/* forward declarations */
typedef struct bcp_struct *bcp;		// problem structure, required as first argument for almost all functions 
typedef __m128i *bc;		// a single boolean cube is a vector of __m128i objects. The size of this vector is stored in the "blk_cnt" member of bcp
typedef struct bcl_struct *bcl;		// boolean cube list
typedef struct bcx_struct *bcx;		// abstract syntax tree of a boolean cube expresion


/* boolean cube problem, each function will require a pointer to this struct */
#define BCP_MAX_STACK_FRAME_DEPTH 500
struct bcp_struct
{
  int var_cnt;  // number of variables per cube
  int blk_cnt;  // number of blocks per cube, one block is one __m128i = 64 variables
  int vars_per_blk_cnt; // number of variables per block --> 64, because one variable requires 2 bit, so a __m128i can hold 64 variables
  int bytes_per_cube_cnt; // number of bytes per cube, this is blk_cnt*sizeof(__m128i)
  char *cube_to_str;    // storage area for one visual representation of a cube
  bcl stack_cube_list;    // storage area for temp cubes
  int stack_frame_pos[BCP_MAX_STACK_FRAME_DEPTH];
  int stack_depth;
  bcl global_cube_list;    // storage area for temp cubes, with the first four fixed cubes: idx 0: all illegal (00), idx 1: all zero (01), idx 2: all one (10) and idx 3: all don't care (11)

  bcl exclude_group_list;       // list of exclusive variable lists, see bcp_DoBCLCubeExcludeGroup(), idea is to auto apply bcp_DoBCLCubeExcludeGroup() for all members of this list
  
  int x_true;           // symbol for true --> '1'
  int x_false;           // symbol for false --> '0'
  int x_end;
  int x_not;
  int x_and;
  int x_or;
  int x_var_cnt;         // variable counter for expressions, should be equal to coMapSize(var_map)
  
	
  clock_t clock_do_bcl_multi_cube_containment;		// max time limit given for multi cube containment operation, must be seconds*CLOCKS_PER_SEC
	
  co var_map;           // map with all variables, key=name, value=position (double), p->x_var_cnt contains the number of variables in var_map
  co var_list;                  // vector with all variables, derived from var_map, use coPrint(p->var_list); puts(""); to print the variables
};

/* a list of boolean cubes */
struct bcl_struct
{
  volatile int cnt;       // there are algoritms which may modify the cnt, so tell the compiler to always get this data from the structure
  int max;
  int last_deleted;
  __m128i *list;        // max * var_cnt / 64 entries
  uint8_t *flags;       // bit 0 is the cube deleted flag
};

/* boolean cube expression */
#define BCX_TYPE_NONE 0
#define BCX_TYPE_ID 1
#define BCX_TYPE_NUM 2
#define BCX_TYPE_AND 3
#define BCX_TYPE_OR 4
#define BCX_TYPE_BCL 5

struct bcx_struct
{
  int type;				// One of the BCX_TYPE_xxx definitions
  int is_not;           // 0 or 1
  bcx next;
  bcx down;
  int val;
  char *identifier;
  bcl cube_list;
};

/*============================================================*/

/* bcutil.c */
extern int bc_log_level;
void print128_num(__m128i var);
void logprint(int log, const char *fmt, ...);


/* bcp.c */
int bcp_GetVarCntFromString(const char *s);

bcp bcp_New(size_t var_cnt);
int bcp_UpdateFromBCX(bcp p);
void bcp_Delete(bcp p);
//bc bcp_GetGlobalCube(bcp p, int pos);
#define bcp_GetGlobalCube(p, pos) \
  bcp_GetBCLCube((p), (p)->global_cube_list, (pos))  
void bcp_CopyGlobalCube(bcp p, bc r, int pos);
void bcp_StartCubeStackFrame(bcp p);
void bcp_EndCubeStackFrame(bcp p);
bc bcp_GetTempCube(bcp p);	// requires bcp_StartCubeStackFrame()

/* bcube.c */
/* core functions */

void bcp_ClrCube(bcp p, bc c);
void bcp_CopyCube(bcp p, bc dest, bc src);
int bcp_CompareCube(bcp p, bc a, bc b);


void bcp_SetCubeVar(bcp p, bc c, unsigned var_pos, unsigned value);
//unsigned bcp_GetCubeVar(bcp p, bc c, unsigned var_pos);
#define bcp_GetCubeVar(p, c, var_pos) \
  ((((uint16_t *)(c))[(var_pos)/8] >> (((var_pos)&7)*2)) & 3)

const char *bcp_GetStringFromCube(bcp p, bc c);         // returns a pointer to an internal memory area, which must not be free'd. The returned address will be overwritten with a next call to this fn
void bcp_SetCubeByStringPointer(bcp p, bc c,  const char **s);
void bcp_SetCubeByString(bcp p, bc c, const char *s);

/* boolean functions */

int bcp_IsTautologyCube(bcp p, bc c);
void bcp_GetVariableMask(bcp p, bc mask, bc c);
void bcp_InvertCube(bcp p, bc c);  	// invert veriables in the cube, 3 Jun 24: NOT TESTED
int bcp_IsAndZero(bcp p, bc a, bc b);           // check if the bitwise AND is zero, should be used together with bcp_GetVariableMask()
unsigned bcp_OrBitCnt(bcp p, bc r, bc a, bc b);
int bcp_IntersectionCube(bcp p, bc r, bc a, bc b); // returns 0, if there is no intersection
int bcp_IsIntersectionCube(bcp p, bc a, bc b); // returns 0, if there is no intersection
int bcp_IsIllegal(bcp p, bc c);                                 // check whether "c" contains "00" codes
int bcp_GetCubeVariableCount(bcp p, bc cube);   // return the number of 01 or 10 codes in "cube"
int bcp_GetCubeDelta(bcp p, bc a, bc b);                // calculate the delta between a and b
int bcp_IsSubsetCube(bcp p, bc a, bc b);                // is "b" is a subset of "a"

/* bclcore.c */

#define bcp_GetBCLCnt(p, l) ((l)->cnt)

bcl bcp_NewBCL(bcp p);          // create new empty bcl
bcl bcp_NewBCLByBCL(bcp p, bcl l);      // create a new bcl as a copy of an existing bcl
bcl bcp_NewBCLWithCube(bcp p, int global_cube_pos); // creare new bcl and copy one cube from global cube list into it
int bcp_CopyBCL(bcp p, bcl a, bcl b);   // copy content from bcl b into bcl a, return 0 for error
void bcp_ClearBCL(bcp p, bcl l);
void bcp_DeleteBCL(bcp p, bcl l);
int bcp_ExtendBCL(bcp p, bcl l);
//bc bcp_GetBCLCube(bcp p, bcl l, int pos);
#define bcp_GetBCLCube(p, l, pos) \
  (bc)(((uint8_t *)((l)->list)) + (pos) * (p)->bytes_per_cube_cnt)
void bcp_ShowBCL(bcp p, bcl l);
void bcp_Show2BCL(bcp p, bcl l1, bcl l2);
int bcp_IsPurgeUsefull(bcp p, bcl l);
void bcp_PurgeBCL(bcp p, bcl l);               /* purge deleted cubes, deleted cubes are marked in the flags with a 1 */
int bcp_AddBCLCube(bcp p, bcl l); // add empty cube to list l, returns the position of the new cube or -1 in case of error
int bcp_AddBCLCubeByCube(bcp p, bcl l, bc c); // append cube c to list l, returns the position of the new cube or -1 in case of error
int bcp_AddBCLCubesByBCL(bcp p, bcl a, bcl b); // append cubes from b to a, does not do any simplification, returns 0 on error
int bcp_AddBCLCubesByString(bcp p, bcl l, const char *s); // add cube(s) described as a string, returns 0 in case of error

bcl bcp_NewBCLByString(bcp p, const char *s);   // create a bcl from a CR seprated list of cubes


int *bcp_GetBCLVarCntList(bcp p, bcl l);

void bcp_SetBCLFlipVariables(bcp p, bcl l);
void bcp_SetBCLAllDCToZero(bcp p, bcl l, bcl extra_mask);

void bcp_AndElementsBCL(bcp p, bcl l, bc result);
void bcp_AndBCL(bcp p, bcl l);


/* bcldimacscnf.c */

bcp bcp_NewByDIMACSCNF(FILE *fp);
bcl bcp_NewBCLByDIMACSCNF(bcp p, FILE *fp);   // create a bcl from a DIMACS CNF


/* bccofactor.c */

void bcp_CalcBCLBinateSplitVariableTable(bcp p, bcl l);
int bcp_GetBCLMaxBinateSplitVariableSimple(bcp p, bcl l);
int bcp_GetBCLMaxBinateSplitVariable(bcp p, bcl l);
int bcp_IsBCLVariableDC(bcp p, bcl l, unsigned var_pos);
int bcp_IsBCLVariableUnate(bcp p, bcl l, unsigned var_pos, unsigned value);
void bcp_DoBCLOneVariableCofactor(bcp p, bcl l, unsigned var_pos, unsigned value);
bcl bcp_NewBCLCofacterByVariable(bcp p, bcl l, unsigned var_pos, unsigned value);       // create a new list, which is the cofactor from "l"
void bcp_DoBCLCofactorByCube(bcp p, bcl l, bc c, int exclude);         
bcl bcp_NewBCLCofactorByCube(bcp p, bcl l, bc c, int exclude);          // don't use this fn, use bcp_IsBCLCubeRedundant() or bcp_IsBCLCubeCovered() instead
int bcp_IsBCLUnate(bcp p);  // requires call to bcp_CalcBCLBinateSplitVariableTable


/* bclcontainment.c */

int bcp_IsBCLCubeSingleCovered(bcp p, bcl l, bc c);
void bcp_DoBCLSingleCubeContainment(bcp p, bcl l);
int bcp_IsBCLCubeCovered(bcp p, bcl l, bc c);           // is cube c a subset of l (is cube c covered by l)
int bcp_IsBCLCubeRedundant(bcp p, bcl l, int pos);      // is the cube at pos in l covered by all other cubes in l
void bcp_DoBCLMultiCubeContainment(bcp p, bcl l);


/* bcltautology.c */

int bcp_is_bcl_partition(bcp p, bcl l);
int bcp_IsBCLTautology(bcp p, bcl l);


/* bclsubtract.c */

// void bcp_DoBCLSharpOperation(bcp p, bcl l, bc a, bc b);
int bcp_SubtractBCL(bcp p, bcl a, bcl b, int is_mcc);  // if is_mcc is 0, then the substract operation will generate all prime cubes. returns 0 for error

/* bclcomplement.c */

bcl bcp_NewBCLComplementWithSubtract(bcp p, bcl l);  // faster than with cofactor
bcl bcp_NewBCLComplementWithCofactor(bcp p, bcl l); // slow!
bcl bcp_NewBCLComplement(bcp p, bcl l);         // calls bcp_NewBCLComplementWithSubtract();
bcl bcp_NewBCLComplementWithIntersection(bcp p, bcl l);
int bcp_ComplementBCL(bcp p, bcl l);            // in place complement calculation


/* bclsubset.c */

int bcp_IsBCLSubsetWithCofactor(bcp p, bcl a, bcl b);   //   test, whether "b" is a subset of "a", returns 1 if this is the case
int bcp_IsBCLSubsetWithSubtract(bcp p, bcl a, bcl b);  // this fn seems to be much slower than bcp_IsBCLSubsetWithCofactor
int bcp_IsBCLSubset(bcp p, bcl a, bcl b);       //   test, whether "b" is a subset of "a", calls bcp_IsBCLSubsetWithCofactor()
int bcp_IsBCLEqual(bcp p, bcl a, bcl b);                // checks whether the two lists are equal


/* bclintersection.c */

int bcp_IntersectionBCLs(bcp p, bcl result, bcl a, bcl b); // result = a intersection with b
int bcp_IntersectionBCL(bcp p, bcl a, bcl b);   // a = a intersection with b 

/* bclexpand.c */

void bcp_DoBCLSimpleExpand(bcp p, bcl l);
void bcp_DoBCLExpandWithOffSet(bcp p, bcl l, bcl off);  // this operation does not do any SCC or MCC
void bcp_DoBCLExpandWithCofactor(bcp p, bcl l);

/* bclminimize.c */

void bcp_MinimizeBCL(bcp p, bcl l);                             // minimize l
void bcp_MinimizeBCLWithOnSet(bcp p, bcl l);

/* bcexpression.c */

/*
  bcp p;
  bcx x;
  bcl l;
  p = bcp_New(0);                                                               // create a new (incomplete) problem structure
  x = bcp_Parse(p, expression, is_not_propagation, 1);          // parse a string expression, this will also update p>x_var_cnt, can be called multiple times before calling bcp_UpdateFromBCX()
  bcp_UpdateFromBCX(p);                                                      // takeover the variables from the expression into the problem structure
  l = bcp_NewBCLByBCX(p, x);                                            // get the bcl from x
  bcp_DeleteBCX(p, x);                                                          // remove the expression

  if p is already filled with all the variables, then use 
    bcx x = bcp_Parse(p, expr_str, 1, 0);              // assumption: p already contains all variables of expr_str, variables not in p are ignored
    bcl l = bcp_NewBCLByBCX(p, x);          // create a bcl from the expression tree 
    bcp_DeleteBCX(p, x);                      // free the expression tree

*/

void bcp_skip_space(bcp p, const char **s);
const char *bcp_get_identifier(bcp p, const char **s);
int bcp_get_value(bcp p, const char **s);

bcx bcp_NewBCX(bcp p);
void bcp_DeleteBCX(bcp p, bcx x);

int bcp_AddVar(bcp p, const char *s);   // used in bcexpression.c but also in bcjson.c

//int bcp_AddVarsFromBCX(bcp p, bcx x);
//int bcp_BuildVarList(bcp p);

bcx bcp_Parse(bcp p, const char *s, int is_not_propagation, int is_register_variables); // is_not_propagation: Remove any not within the tree

void bcp_ShowBCX(bcp p, bcx x);
void bcp_PrintBCX(bcp p, bcx x);

bcl bcp_NewBCLMinTermByVarList(bcp p, const char *s);   // blank separated list of var names, "1" if var exists, "0" if not, returns bcl with exactly one cube (=minterm)

bcl bcp_NewBCLByBCX(bcp p, bcx x);

char *bcp_GetExpressionBCL(bcp p, bcl l);       // convert "l" to a human readable expression, return value must be free'd if not NULL

/* bcexclude.c */

int bcl_ExcludeBCLVars(bcp p, bcl l, bcl grp);                  // obsolete, replaced by bcp_DoBCLExcludeGroup()
int bcp_DoBCLExcludeGroup(bcp p, bcl l, bc grp);                // 
int bcp_DoBCLExcludeGroupList(bcp p, bcl l, bcl grp_list);
int bcp_DoBCLXGroup(bcp p, bcl l);


/* bcjson.c */

#define SLOT_CNT 99
#define CREATE_STR(a) #a
#define CREATE_STR2(a) CREATE_STR(a)
#define SLOT_CNT_STR CREATE_STR2(SLOT_CNT)


int bc_ExecuteJSON(FILE *in_fp, FILE *out_fp, int isCompactJSONOutput);
int bc_ExecuteJSONFiles(const char **json_input_filenames, int input_file_cnt, const char *json_output_filename, int isCompact);



/* bcselftest.c */

bcl bcp_NewBCLWithRandomTautology(bcp p, int size, int dc2one_conversion_cnt);
void internalTest(int var_cnt);
void speedTest(int var_cnt);
void minimizeTest(int cnt);
void expressionTest(void);
void excludeTest(void);



#endif

