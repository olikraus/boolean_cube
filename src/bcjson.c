/*

  bcljson.c
  
  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

  
  
  [
    {
      label:"",
      label0:"",
      cmd:"",
      bcl:"",
      slot:n
    }     
  ]

  keys of the vector map:

    label/label0
      Output the current flags. If label0 is used, then also output the content of slot0.
      Using the label member is the preffered way of generating output and results.
      
    cmd
      A command as described below.
      
    bcl
      A boolean cube list, which is used as an argument to "cmd".
      
    expr
      Expression, which is parsed and converted into a bcl
      if both "bcl" and "expr" are present, then "bcl" is used and the expr is ignored
      
    slot
      Another argument for some of the "cmd"s below. Defaults to 0.

  Note: slot always defaults to 0

  values for "cmd"

    bcl2slot
      load the provided bcl into the given slot
      { "cmd":"bcl2slot", "bcl":"110-", "slot":0 }

    show
      if bcl/expr is present, then show bcl/expr
        { "cmd":"show", "bcl":"0011" }
      if slot is present, then show the slot content
        { "cmd":"show", "slot":0 }
      This command is for debugging only, use label/label0 for proper output

    intersection0
      Calculate intersection and store the result in slot 0
      This operation will set the "empty" flag.
      if bcl/expr is present, then calculate intersection between slot 0 and bcl/expr.
        { "cmd":"intersection0", "bcl":"11-0" }
      if slot is present, then calculate intersection between slot 0 and the provided slot
        { "cmd":"intersection0", "slot":1 }
    
    subtract0
      Subtract a bcl/expr/slot from slot 0 and store the result in slot 0
      This operation will set the "empty" flag.
      if bcl/expr is present, then calculate slot 0 minus bcl/expr.
        { "cmd":"subtract0", "bcl":"11-0" }
      if slot is present, then calculate slot 0 minus the given slot.
        { "cmd":"subtract0", "slot":1 }

    equal0
      Compare a bcl/expr/slot with slot 0 and and set the superset and subset flags.
      If both flags are set to 1, then the given bcl/slot is equal to slot 0
      
    

    exchange0
      Exchange slot 0 and the given other slot
        { "cmd":"exchange0", "slot":1 }
        
    copy0
      Copy slot 0 and the given other slot
        { "cmd":"copy0", "slot":1 }


*/

#include "co.h"
#include "bc.h"
#include <string.h>
#include <assert.h>

#define SLOT_CNT 10

/*
  takes a JSON input (as co object) and produces a JSON output (again as co object)
*/
co bc_ExecuteVector(cco in)
{
  bcp p = NULL;
  bcl slot_list[SLOT_CNT];
  int var_cnt = -1;
  long cnt = coVectorSize(in);
  long i;
  cco o;
  const char *cmd = NULL;
  const char *label = NULL;
  const char *label0 = NULL;
  int slot = 0;
  const char *bclstr = NULL;
  bcl l = NULL;
  bcl arg;              // argument.. either l or a slot
  int result = 0;
  int is_empty = -1;
  int is_0_superset = -1;
  int is_0_subset = -1;
  co output = coNewMap(CO_STRDUP|CO_STRFREE|CO_FREE_VALS);
  
  assert( output != NULL );
  
  for( i = 0; i < SLOT_CNT; i++ )
    slot_list[i] = NULL;
  
  assert( coIsVector(in) );

  // PRE PHASE: Collect all variable names
  for( i = 0; i < cnt; i++ )
  {
    cco cmdmap = coVectorGet(in, i);
    if ( coIsMap(cmdmap) )
    {
      o = coMapGet(cmdmap, "xend");
      if (coIsStr(o))
      {
        if ( p == NULL ) { p = bcp_New(0); assert( p != NULL ); }
        if ( strlen(coStrGet(o)) > 0 )
          p->x_end = coStrGet(o)[0];
      }

      o = coMapGet(cmdmap, "xand");
      if (coIsStr(o))
      {
        if ( p == NULL ) { p = bcp_New(0); assert( p != NULL ); }
        if ( strlen(coStrGet(o)) > 0 )
          p->x_and = coStrGet(o)[0];
      }

      o = coMapGet(cmdmap, "xor");
      if (coIsStr(o))
      {
        if ( p == NULL ) { p = bcp_New(0); assert( p != NULL ); }
        if ( strlen(coStrGet(o)) > 0 )
          p->x_or = coStrGet(o)[0];
      }

      o = coMapGet(cmdmap, "xnot");
      if (coIsStr(o) && p != NULL )
      {
        if ( p == NULL ) { p = bcp_New(0); assert( p != NULL ); }
        if ( strlen(coStrGet(o)) > 0 )
          p->x_not = coStrGet(o)[0];
      }
        
      o = coMapGet(cmdmap, "expr");             // "expr" is an alternative way to describe a bcl
      if (coIsStr(o))                     // only of the expr is a string and only of the bcl has not been assigned before
      {
        const char *expr_str = coStrGet(o);        
        bcx x;
        if ( p == NULL )
        {
          p = bcp_New(0);               // create a dummy bcp
          assert( p != NULL );
        }
        x = bcp_Parse(p, expr_str, 0);              // no propagation required, we are just collecting the names
        if ( x != NULL )
            bcp_DeleteBCX(p, x);                      // free the expression tree
      }
    }
  }
  
  if ( p != NULL )      // if a dummy bcp had been created, then convert that bcp to a regular bcp
  {
      bcp_UpdateFromBCX(p);
  }

  
  // MAIN PHASE: Execute each element of the json array  
  
  for( i = 0; i < cnt; i++ )
  {
    cco cmdmap = coVectorGet(in, i);
    cmd = "";
    label = NULL;
    label0 = NULL;
    slot = 0;
    is_empty = -1;
    if ( l != NULL )
      bcp_DeleteBCL(p, l);
    l = NULL;
    if ( coIsMap(cmdmap) )
    {
      
      // STEP 1: Read the member variables from a cmd block: "cmd", "slot", "bcl"

        
      o = coMapGet(cmdmap, "cmd");
      if (coIsStr(o))
        cmd = coStrGet(o);

      o = coMapGet(cmdmap, "label");
      if (coIsStr(o))
        label = coStrGet(o);

      o = coMapGet(cmdmap, "label0");
      if (coIsStr(o))
        label0 = coStrGet(o);

      o = coMapGet(cmdmap, "slot");
      if (coIsDbl(o))
      {
        slot = (int)coDblGet(o);
        if ( slot >= SLOT_CNT || slot < 0 )
          slot = 0;
      }
      
      o = coMapGet(cmdmap, "bcl");
      if (coIsStr(o))
      {
        bclstr = coStrGet(o);
        if ( p == NULL )
        {
          var_cnt = bcp_GetVarCntFromString(bclstr);
          if ( var_cnt > 0 )
          {
            p = bcp_New(var_cnt);
            assert( p != NULL );
          }
        }
        l = bcp_NewBCLByString(p, bclstr);
        assert( l != NULL );
      }
      else if ( coIsVector(o) )
      {
        long i;
        for( i = 0; i < coVectorSize(o); i++ )
        {
          cco so = coVectorGet(o, i);
          if (coIsStr(so))
          {
            bclstr = coStrGet(so);
            if ( p == NULL )
            {
              var_cnt = bcp_GetVarCntFromString(bclstr);
              if ( var_cnt > 0 )
              {
                p = bcp_New(var_cnt);
                assert( p != NULL );
              }
            } // p == NULL
            if ( l == NULL )
            {
              l = bcp_NewBCL(p);
              assert( l != NULL );
            } // l == NULL
            result = bcp_AddBCLCubesByString(p, l, bclstr);
            assert( result != 0 );
            
          } // coIsStr
        } // for
      } // bcl is vector

      o = coMapGet(cmdmap, "expr");             // "expr" is an alternative way to describe a bcl
      if (coIsStr(o) && l == NULL )                     // only of the expr is a string and only of the bcl has not been assigned before
      {
        const char *expr_str = coStrGet(o);        
        //printf("expr: %s\n", expr_str);
        bcx x = bcp_Parse(p, expr_str, 1);              // assumption: p already contains all variables of expr_str
        if ( x != NULL )
        {
            l = bcp_NewBCLByBCX(p, x);          // create a bcl from the expression tree 
            bcp_DeleteBCX(p, x);                      // free the expression tree
        }
      }

      // STEP 2: Execute the command
      
      arg = (l!=NULL)?l:slot_list[slot];

      // "bcl2slot"  "bcl" into "slot"
      if ( p != NULL && strcmp(cmd, "bcl2slot") == 0 )
      {
        if ( l != NULL )
        {
          if ( slot_list[slot] != NULL )
            bcp_DeleteBCL(p, slot_list[slot]);
          slot_list[slot] = l;
          l = NULL;
        }
      }
      else if ( p != NULL &&  strcmp(cmd, "minimize") == 0 )
      {
        assert(arg != NULL);
        bcp_MinimizeBCL(p, arg);
      }
      else if ( p != NULL &&  strcmp(cmd, "complement") == 0 )
      {
        assert(arg != NULL);
        bcp_ComplementBCL(p, arg);
      }
      // "show"  "bcl" or "show" bcl from "slot"
      else if ( p != NULL &&  strcmp(cmd, "show") == 0 )
      {
        assert(arg != NULL);
        printf("cmd=%s label=%s label0=%s\n", cmd, label, label0);
        bcp_ShowBCL(p, arg);
      }
      // intersection0: calculate intersection with slot 0
      // result is stored in slot 0
      else if ( p != NULL &&  strcmp(cmd, "intersection0") == 0 )
      {
        int intersection_result;
        assert(slot_list[0] != NULL);
        assert(arg != NULL);
        bcp_IntersectionBCL(p, slot_list[0], arg);   // a = a intersection with b 
        assert(intersection_result != 0);
        is_empty = 0;
        if ( slot_list[0]->cnt == 0 )
          is_empty = 1;
      }
      else if ( p != NULL &&  strcmp(cmd, "subtract0") == 0 )
      {
        int subtract_result;
        assert(slot_list[0] != NULL);
        assert(arg != NULL);
        bcp_SubtractBCL(p, slot_list[0], arg, 1);   // a = a minus b 
        assert(subtract_result != 0);
        is_empty = 0;
        if ( slot_list[0]->cnt == 0 )
          is_empty = 1;
      }
      else if ( p != NULL &&  strcmp(cmd, "equal0") == 0 )
      {
        assert(slot_list[0] != NULL);
        assert(arg != NULL);
        is_0_superset = bcp_IsBCLSubset(p, slot_list[0], arg);       //   test, whether "arg" is a subset of "slot_list[0]": 1 if slot_list[0] is a superset of "arg"
        is_0_subset = bcp_IsBCLSubset(p, arg, slot_list[0]);
        is_empty = 0;
        if ( slot_list[0]->cnt == 0 )
          is_empty = 1;
      }
      else if ( p != NULL &&  strcmp(cmd, "exchange0") == 0 )
      {
        bcl tmp;
        assert(slot_list[0] != NULL);
        assert(slot_list[slot] != NULL);
        tmp = slot_list[slot];
        slot_list[slot] = slot_list[0];
        slot_list[0] = tmp;
      }
      else if ( p != NULL &&  strcmp(cmd, "copy0") == 0 )
      {
        if ( slot_list[slot] == NULL )
          slot_list[slot] = bcp_NewBCL(p);
        assert( slot_list[slot] != NULL );
        assert( slot_list[0] != NULL );        
        bcp_CopyBCL(p, slot_list[slot], slot_list[0]);
      }
      

      // STEP 3: Generate JSON output
    
      if ( label != NULL || label0 != NULL )
      {
        co e = coNewMap(CO_STRDUP|CO_STRFREE|CO_FREE_VALS);
        assert( e != NULL );
        coMapAdd(e, "index", coNewDbl(i));
        //coMapAdd(e, "label", coNewStr(CO_STRDUP, label));
        
        if ( is_empty >= 0 )
        {
          coMapAdd(e, "empty", coNewDbl(is_empty));          
        }

        if ( is_0_superset >= 0 )
        {
          coMapAdd(e, "superset", coNewDbl(is_0_superset));          
        }

        if ( is_0_subset >= 0 )
        {
          coMapAdd(e, "subset", coNewDbl(is_0_subset));          
        }
        
        if ( label0 != NULL && slot_list[0] != NULL )
        {
          int j;
          co v = coNewVector(CO_FREE_VALS);
          for( j = 0; j <  slot_list[0]->cnt; j++ )
          {
            coVectorAdd( v, coNewStr(CO_STRDUP, bcp_GetStringFromCube(p, bcp_GetBCLCube(p, slot_list[0], j))));
          }
          coMapAdd(e, "bcl", v);
          if ( p->x_var_cnt == p->var_cnt )
          {
            coMapAdd(e, "expr", coNewStr(CO_STRFREE, bcp_GetExpressionBCL(p, slot_list[0])));
          }
        }
        
        coMapAdd(output, label0 != NULL?label0:label, e);
      } // label
    } // isMap
    
  } // for all cmd maps
  
  
  {
    co e = coNewMap(CO_STRDUP|CO_STRFREE|CO_FREE_VALS);
    assert( e != NULL );
    coMapAdd(e, "vmap", coClone(p->var_map));   // memory leak !!!
    coMapAdd(e, "vlist", coClone(p->var_list));
    coMapAdd(output, "", e);
  }
  
  
  // Memory cleanup
  
  for( i = 0; i < SLOT_CNT; i++ )
    if ( slot_list[i] != NULL )
      bcp_DeleteBCL(p, slot_list[i]);
    
  if ( l != NULL )
      bcp_DeleteBCL(p, l);

  bcp_Delete(p);
  
  return output;
}

int bc_ExecuteJSON(FILE *in_fp, FILE *out_fp, int isCompactJSONOutput)
{
  co in = coReadJSONByFP(in_fp);
  co out;
  if ( in == NULL )
    return puts("JSON read errror"), 0;
  out = bc_ExecuteVector(in);
  // coWriteJSON(cco o, int isCompact, int isUTF8, FILE *fp); // isUTF8 is 0, then output char codes >=128 via \u 
  coWriteJSON(out, isCompactJSONOutput, 1, out_fp); // isUTF8 is 0, then output char codes >=128 via \u 
  coDelete(out);
  coDelete(in);
  return 1;
}

