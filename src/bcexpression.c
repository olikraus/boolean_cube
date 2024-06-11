/*

  bcexpression.c

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/


*/
#include "bc.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


/*============================================================*/
/* forward declaration */
bcx bcp_ParseOR(bcp p, const char **s);

/*============================================================*/

bcx bcp_NewBCX(bcp p)
{
  bcx x = (bcx)malloc(sizeof(struct bcx_struct));
  if ( x != NULL )
  {
    x->type = BCX_TYPE_NONE;
    x->is_not = 0;
    x->next = NULL;
    x->down = NULL;
    x->val = 0;
    x->identifier = 0;
    x->cube_list = NULL;
    return x;
  }
  return NULL;
}

void bcp_DeleteBCX(bcp p, bcx x)
{
  if ( x == NULL )
    return;
  bcp_DeleteBCX(p, x->down);
  bcp_DeleteBCX(p, x->next);
  x->type = BCX_TYPE_NONE;
  if ( x->identifier != NULL )
  {
    free(x->identifier);
    x->identifier = NULL;
  }
  if ( x->cube_list != NULL )
  {
    bcp_DeleteBCL(p, x->cube_list);
    x->cube_list = NULL;
  }
  free(x);
}


bcx bcp_NewBCXValue(bcp p, int v)
{
  bcx x = bcp_NewBCX(p);
  if ( x != NULL )
  {
    x->type = BCX_TYPE_NUM;
    x->val = v;
    return x;
  }
  return NULL;
}

bcx bcp_NewBCXIdentifier(bcp p, const char *identifier)
{
  bcx x = bcp_NewBCX(p);
  if ( x != NULL )
  {
    x->type = BCX_TYPE_ID;
    x->identifier = strdup(identifier);
    if ( x->identifier != NULL )
    {
      return x;
    }
    bcp_DeleteBCX(p, x);
  }
  return NULL;
}



void bcp_ParseError(bcp p, const char *error_msg)
{
  puts(error_msg);
}


/*============================================================*/

void bcp_skip_space(bcp p, const char **s)
{
  for(;;)
  {
    if ( **s == '\0' )
      break;
    if ( **s == p->x_end )
      break;
    if ( **s > 32 )
      break;
    (*s)++;
  }
}

#define BCP_IDENTIFIER_MAX 1024
const char *bcp_get_identifier(bcp p, const char **s)
{
  static char identifier[BCP_IDENTIFIER_MAX];
  int i = 0;
  identifier[0] = '\0';
  //if ( isalpha(**s) || **s == '_' )
  if ( isalnum(**s) || **s == '_' )
  {
    for(;;)
    {
      if ( **s == '\0' || **s == p->x_end )
        break;
      if ( isalnum(**s) || **s == '_' )
      {
        if ( i+1 < BCP_IDENTIFIER_MAX )
          identifier[i++] = **s;
        (*s)++;
      }
      else
      {
        break;
      }
    }
  }
  identifier[i] = '\0';
  bcp_skip_space(p, s);
  return identifier;
}

int bcp_get_value(bcp p, const char **s)
{
  int v = 0;
  if ( isdigit(**s) )
  {
    for(;;)
    {
      if ( **s == '\0' || **s == p->x_end )
        break;
      if ( isdigit(**s) )
      {
        v = (v*10) + (**s) - '0';
        (*s)++;
      }
      else
      {
        break;
      }
    } // for
  }
  bcp_skip_space(p, s);
  return v;
}

/*============================================================*/

bcx bcp_ParseAtom(bcp p, const char **s)
{
  static char msg[32];
  bcx x;

  if ( **s == '\0' || **s == p->x_end )    // this is reached if the string is full empty
  {
    return bcp_NewBCXValue(p, 0);       // return 0 value if the string is (unexpectedly) empty
  }
  else if ( **s == '(' )
  {
    (*s)++;
    bcp_skip_space(p, s);
    x = bcp_ParseOR(p, s);
    if ( x == NULL )
      return NULL;
    if ( **s != ')' )
    {
      bcp_ParseError(p, "Missing ')'");
      bcp_DeleteBCX(p, x);
      return NULL;
    }
    (*s)++;
    bcp_skip_space(p, s);
    return x;
  }
  else if ( **s == p->x_false )
  {
    return bcp_NewBCXValue(p, 0);
  }
  else if ( **s == p->x_true )
  {
    return bcp_NewBCXValue(p, 1);
  }
  //else if ( isdigit(**s) )
  //{
  //  return bcp_NewBCXValue(p, bcp_get_value(p, s));
  //}
  //else if ( isalpha(**s) || **s == '_' )
  else if ( isalnum(**s) || **s == '_' )
  {
    return bcp_NewBCXIdentifier(p, bcp_get_identifier(p, s) );
  }
  else if ( **s == p->x_not )
  {
    (*s)++;
    bcp_skip_space(p, s);
    x = bcp_ParseAtom(p, s);
    if ( x == NULL )
      return NULL;
    x->is_not = !x->is_not;
    return x;
  }
  sprintf(msg, "Unknown char '%c'", **s);       // 17
  bcp_ParseError(p, msg);
  return NULL;
}

bcx bcp_ParseAND(bcp p, const char **s)
{
  bcx x = bcp_ParseAtom(p, s);
  bcx binop, xx;
  if ( x == NULL )
    return NULL;
  if ( **s != p->x_and )
    return x;
  
  binop = bcp_NewBCX(p);
  binop->type = BCX_TYPE_AND;
  binop->down = x;
  
  while ( **s == p->x_and )
  {
    (*s)++;
    bcp_skip_space(p, s);
    xx = bcp_ParseAtom(p, s);
    if ( xx == NULL )
    {
      bcp_DeleteBCX(p, binop);
      return NULL;
    }
    x->next = xx;
    x = xx;
  }
  
  return binop;
}


bcx bcp_ParseOR(bcp p, const char **s)
{
  bcx x = bcp_ParseAND(p, s);
  bcx binop, xx;
  if ( x == NULL )
    return NULL;
  if ( **s != p->x_or )
    return x;
  
  binop = bcp_NewBCX(p);
  binop->type = BCX_TYPE_OR;
  binop->down = x;
  
  while ( **s == p->x_or )
  {
    (*s)++;
    bcp_skip_space(p, s);
    xx = bcp_ParseAND(p, s);
    if ( xx == NULL )
    {
      bcp_DeleteBCX(p, binop);
      return NULL;
    }
    x->next = xx;
    x = xx;
  }
  
  return binop;
}


/*============================================================*/

/*
  add variable "s" to p->var_map and assign the position as value to the variable name
    key: variable name
    value: position number, starting with 0
*/
int bcp_AddVar(bcp p, const char *s)
{
  if ( p->var_map == NULL )
  {
    p->var_map = coNewMap(CO_STRDUP|CO_STRFREE|CO_FREE_VALS);
    if ( p->var_map == NULL )
      return 0;
  }  
  if ( coMapExists(p->var_map, s) )
    return 1;
  return coMapAdd(p->var_map, s, coNewDbl(p->x_var_cnt++));
}

/*
  Description:
    Register all variables in the expression "x" into the boolean cube problem p
  Args:
    p           Boolean Problem Object 
    x           Expression
*/
int bcp_AddVarsFromBCX(bcp p, bcx x)
{
  if ( x == NULL )
    return 1;
  if ( x->type == BCX_TYPE_ID )
    bcp_AddVar(p, x->identifier);
  if ( bcp_AddVarsFromBCX(p, x->down) == 0 )
    return 0;
  return bcp_AddVarsFromBCX(p, x->next);
}

/* 
  build var_list from var_map 

  The value from var_map is the index position for var_list.
  This means:
    for a given string s
      var_list[var_map[s]] == s
    and for a given index:
      var_map[var_list[index]] == index

  p->var_list is only required, if we want to convert back the BCL to a human 
  readable expressions

  This function is called by "char *bcp_GetExpressionBCL(bcp p, bcl l)", so there should be no need to call this fn directly

*/
int bcp_BuildVarList(bcp p)
{
  int i;
  coMapIterator iter;
  if ( p->var_map == NULL ) // check if there are any variables 
    return 1;   // all ok, if there are no variables
  
  if ( p->var_list == NULL )
  {
    p->var_list = coNewVector(CO_FREE_VALS);
    if ( p->var_list == NULL )
      return 0;
  }
  coVectorClear(p->var_list);
  for( i = 0; i < p->x_var_cnt; i++ )
  {
    if ( coVectorAdd(p->var_list, NULL) < 0 )
      return 0;
  }
  

  if ( coMapLoopFirst(&iter, p->var_map) )
  {
    do 
    {
      i = coDblGet(coMapLoopValue(&iter));
      coVectorSet( p->var_list, i, coNewStr(0, coMapLoopKey(&iter)) );
    } while( coMapLoopNext(&iter) );
  }
  
  return 1;
}

/*============================================================*/

/*
  Apply deMorgan low, so that the is_not flag is only set to any leaf node
  This will avoid BCL complement calculation in bcp_NewBCLByBCX()
  This function will change the expression argument "x"
*/
static void bcp_PropagateNotBCX(bcp p, bcx x)
{
  if ( x == NULL )
    return;

  if ( x->is_not )
  {
    bcx xx = x->down;
    // apply de morgan if required
    switch(x->type)
    {
      case BCX_TYPE_AND:
        x->type = BCX_TYPE_OR;
        x->is_not = 0;
        while( xx != NULL )
        {
          xx->is_not = !xx->is_not;       // invert the not flag of the child
          xx = xx->next;
        }
        break;
      case BCX_TYPE_OR:
        x->type = BCX_TYPE_AND;
        x->is_not = 0;
        while( xx != NULL )
        {
          xx->is_not = !xx->is_not;       // invert the not flag of the child
          xx = xx->next;
        }
        break;        
    }
  }  
  
  bcp_PropagateNotBCX(p, x->down);
  bcp_PropagateNotBCX(p, x->next);
  
}

/*============================================================*/

/*
  Parse a given textual expression and return the abtract syntax tree
  of that expression (bcx object).
  The return value must be free'd with  "bcp_DeleteBCX()"

  Overall procedure
    1. create a dummy bcp 
    2. with all textual expressions: parse the expression and delete the resulting bcx
          --> this will add all variables to bcp
    3. make a call to bcp_UpdateFromBCX(p) so that the bcp matches the variable cnt from all the expressions
    4. with all textual expressions: parse the expresion, get the BCL and delete the expression

  is_not_propagation:
    0: do not apply de morgan rules to the expression try
    1: apply de morgan rules to the expression tree and move all not to the leafs of the tree (strongly recommended!!!)

*/
bcx bcp_Parse(bcp p, const char *s, int is_not_propagation, int is_register_variables)
{
  bcx x;
  // prepare the expression, so that we can start with the parsing
  const char **t = &s; 
  bcp_skip_space(p, t);    
  // convert the given textual expression into an abstract syntax tree
  x = bcp_ParseOR(p, t);

  // bcp_ShowBCX(p, x);puts("");
  // bcp_PrintBCX(p, x);puts("");
  
  // add all variables from the expression to problem record
  if ( is_register_variables )
    if ( bcp_AddVarsFromBCX(p, x) == 0 )
      return bcp_DeleteBCX(p, x), NULL;

  // move any "not" to the leafs by applying de morgan law
  // this will avoid calculation of complements inside bcl bcp_NewBCLByBCX(bcp p, bcx x)
  // this call is optional
  bcp_PropagateNotBCX(p, x);   
  
  // finally return the expresson tree
  return x;
}

/*============================================================*/

void bcp_ShowBCX(bcp p, bcx x)
{
  int is_not;
  if ( x == NULL )
    return;
  is_not = x->is_not;
  if ( is_not )
  {
    printf("%c", p->x_not);
  }
  switch(x->type)
  {
    case BCX_TYPE_ID:
      printf("%s", x->identifier);
      break;
    case BCX_TYPE_NUM:
      printf("%d", x->val);
      break;
    case BCX_TYPE_AND:
      x = x->down;
      printf("(");
      while( x != NULL )
      {
        bcp_ShowBCX(p, x);
        if ( x->next == NULL )
          break;
        printf("%c", p->x_and);
        x = x->next;
      }
      printf(")");
      break;
    case BCX_TYPE_OR:
      x = x->down;
      printf("(");
      while( x != NULL )
      {
        bcp_ShowBCX(p, x);
        if ( x->next == NULL )
          break;
        printf("%c", p->x_or);
        x = x->next;
      }
      printf(")");
      break;
    case BCX_TYPE_BCL:
      printf("BCL");
      break;
    default:
      break;
  }
}

/*============================================================*/

void bcp_PrintBCX(bcp p, bcx x)
{
  if ( x == NULL )
    return;
  printf("%p: t=%d id=%s d=%p n=%p\n", x, x->type, x->identifier,  x->down, x->next);
  bcp_PrintBCX(p, x->down);
  bcp_PrintBCX(p, x->next);
}

/*============================================================*/


bcl bcp_NewBCLMinTermByVarList(bcp p, const char *s)    
{
  const char **t = &s;
  const char *var;
  int cube_pos;
  int var_pos;
  cco var_pos_co;
  bcl l;
  
  bcp_skip_space(p, t);
  l = bcp_NewBCL(p);
  if ( l != NULL )
  {
    cube_pos = bcp_AddBCLCube(p, l);
    if ( cube_pos >= 0 )
    {
      bcp_CopyGlobalCube(p, bcp_GetBCLCube(p, l, cube_pos), 1); // all zero
      for(;;)
      {
        if ( **t == '\0' )
          break;
        var = bcp_get_identifier(p, t);
        if ( var == NULL )
          break;
        if ( var[0] == '\0' )
          break;

        var_pos_co = coMapGet(p->var_map, var);
        if ( var_pos_co != NULL )
        {
          assert( coIsDbl(var_pos_co) );
          var_pos = (int)coDblGet(var_pos_co);
          assert( var_pos < p->var_cnt ); // check whether bcp_UpdateFromBCX() was called
          bcp_SetCubeVar(p, bcp_GetBCLCube(p, l, cube_pos), var_pos, 2);
        }
      }
    }
    else
    {
      bcp_DeleteBCL(p, l);
      l = NULL;
    }
  }
  return l;
}


/*============================================================*/

bcl bcp_NewBCLById(bcp p, int is_not, const char *identifier)
{
  bcl l;
  int cube_pos;
  cco var_pos_co;
  unsigned var_pos;
  
  l = bcp_NewBCL(p);
  if ( l != NULL )
  {
    cube_pos = bcp_AddBCLCube(p, l);
    if ( cube_pos >= 0 )
    {
      var_pos_co = coMapGet(p->var_map, identifier);            // find the variable
      if ( var_pos_co != NULL )
      {
        assert( coIsDbl(var_pos_co) );
        var_pos = (int)coDblGet(var_pos_co);
        assert( var_pos < p->var_cnt ); // check whether bcp_UpdateFromBCX() was called
        bcp_SetCubeVar(p, bcp_GetBCLCube(p, l, cube_pos), var_pos, is_not?1:2);
        return l;
      }
      else      // if the variable is not available, then just return the tautology cube: If we use expressions as variable list, then use the "AND" operation only ("OR" will not work)
      {
        return l;
      }
    }
    bcp_DeleteBCL(p, l);
  }
  return NULL; // error
}

bcl bcp_NewBCLByBCX(bcp p, bcx x)
{
  bcl l;
  int is_not;

  if ( x == NULL )
    return bcp_NewBCL(p); // empty list
  
  is_not = x->is_not;
  
  switch(x->type)
  {
    case BCX_TYPE_ID:
      assert( x->down == NULL );
      return bcp_NewBCLById(p, is_not, x->identifier);
    case BCX_TYPE_NUM:
      assert( x->down == NULL );
      if ( (is_not == 0 && x->val == 0) ||  (is_not != 0 && x->val != 0) )
          return bcp_NewBCL(p); // empty list
      return bcp_NewBCLWithCube(p, 3);  // tautology list
    case BCX_TYPE_AND:
      //bcp_ShowBCX(p, x);
      //puts(" AND");
      x = x->down;
      l = bcp_NewBCLByBCX(p, x);
      //bcp_ShowBCX(p, x);
      //puts(" AND");
      //bcp_ShowBCL(p, l);
    
      x = x->next;
      while( x != NULL )
      {
        bcl ll = bcp_NewBCLByBCX(p, x);
        assert( ll != NULL );
        //bcp_ShowBCX(p, x);
        //puts(" AND");
        //bcp_ShowBCL(p, ll);
        
        bcp_IntersectionBCL(p, l, ll);  // bclintersection.c
        //puts("-->");
        //bcp_ShowBCL(p, l);
        bcp_DeleteBCL(p, ll);
        x = x->next;
      }
      if ( is_not )
      {
        bcl ll = bcp_NewBCLComplement(p, l);
        bcp_DeleteBCL(p, l);
        return ll;
      }
      return l;
    case BCX_TYPE_OR:
      x = x->down;
      l = bcp_NewBCLByBCX(p, x);        // l might contain the tautology cube if a variable is unknown
      x = x->next;
      while( x != NULL )
      {
        bcl ll = bcp_NewBCLByBCX(p, x);         // ll might contain the tautology cube if a variable is unknown
        assert( ll != NULL );
        bcp_AddBCLCubesByBCL(p, l, ll);     
        bcp_DeleteBCL(p, ll);
        bcp_DoBCLSingleCubeContainment(p, l);   // if the tautology cube is included in l, then all other cubes are removed... so do not use OR for variable lists
        x = x->next;
      }
      if ( is_not )
      {
        bcl ll = bcp_NewBCLComplement(p, l);
        bcp_DeleteBCL(p, l);
        return ll;
      }
      return l;
    default:
      printf("illegal type %d\n", x->type);
      return NULL;
  }
  assert(0);
  return NULL;
}

/*============================================================*/
/*
  convert BCL to a textual expression
*/

#define STR_CHAR_EXTEND 32

int str_extend(char **s, size_t *max, size_t extend)
{
  void *p;
  if ( *max == 0 || *s == NULL )
  {
    *s = (char *)malloc(extend);
    if ( *s == NULL )
      return 0;
    *max = extend;
    return 1;
  }
  p = realloc(*s, *max + extend);
  if ( p == NULL )
    return 0;
  *s = (char *)p;
  *max += extend;
  return 1;
}

int str_extend_and_append(char **s, size_t *len, size_t *max, const char *t)
{
  size_t tl = strlen(t);
  if ( *len+tl+1 > *max )
    if ( str_extend(s, max, *len + tl +1 - *max + STR_CHAR_EXTEND) == 0 )
      return 0;
    
  strcpy(*s+*len, t);
  *len += tl;
  return 1;
}

int str_extend_and_append_cube(char **s, size_t *len, size_t *max, bcp p, bc c)
{
  int i, var_cnt = p->var_cnt;
  int value;
  char not_str[2];
  char and_str[2];
  char true_str[2];
  int is_first = 1;
  
  not_str[0] = p->x_not;
  not_str[1] = '\0';
  and_str[0] = p->x_and;
  and_str[1] = '\0';
  true_str[0] = p->x_true;
  true_str[1] = '\0';
  
  
  for( i = 0; i < var_cnt; i++ )
  {
    value = bcp_GetCubeVar(p, c, i);
    if ( value == 1 || value == 2 )
    {
      if ( is_first )
        is_first = 0;
      else
        if ( str_extend_and_append(s, len, max, and_str) == 0 )
          return 0;
        
      if ( value == 1 )
        if ( str_extend_and_append(s, len, max, not_str) == 0 )
          return 0;
      
      if ( str_extend_and_append(s, len, max, coStrGet(coVectorGet( p->var_list, i ))) == 0 )
        return 0;
    }
  }
  if ( is_first )
    if ( str_extend_and_append(s, len, max, true_str) == 0 )
      return 0;
  return 1;
}

int str_extend_and_append_list(char **s, size_t *len, size_t *max, bcp p, bcl l)
{
  int i;
  int is_first = 1;
  char or_str[2];
  or_str[0] = p->x_or;
  or_str[1] = '\0';

  if ( str_extend_and_append(s, len, max, "") == 0 )
    return 0;
  
  for( i = 0; i < l->cnt; i++ )
  {
      if ( is_first )
        is_first = 0;
      else
        if ( str_extend_and_append(s, len, max, or_str) == 0 )
          return 0;
        
    if ( str_extend_and_append_cube(s, len, max, p,bcp_GetBCLCube(p, l, i)) == 0 )
      return 0;
  }
  return 1;
}


char *bcp_GetExpressionBCL(bcp p, bcl l)
{
  size_t len, max;
  char *s;
  
  len = 0; 
  max = 0;
  s = NULL;
  
  if ( p->var_list == NULL )
    bcp_BuildVarList(p);
  
  if ( str_extend(&s, &max, STR_CHAR_EXTEND) == 0 )
    return NULL;
  if ( str_extend_and_append_list(&s, &len, &max, p, l) == 0 )
    return free(s), NULL;
  
  return s;
}

/*============================================================*/
/*
  Background:
    There are cases, where variables do exclude each other.
    Let's assume a1, a2, a3, a4
    If the input clause is "a1", then actually it means "a1" but not a2, a3 and a4, so the correct
    expression would be "a1 & !a2 & !a3 & !a4"
    The same is true for "a1&a3": In this case the actual meaning is "a1 & !a2 & a3 & !a4"

    So the idea is: With all variables of a group of variables, where the variables are not used in the expression: Set those variables to "01" (zero)
    This requires a SOP representation in the BCL (which usually should be the case)

    If such a group is combined with other variables (which is usually the case), then the above "nullifing" should be applied.
    However if there is no such group, then nothing should happen.

    This leads to the following pseudo code:

    input: BCL, group of variables

    used var group = a list of all variables of group, which are used anywhere in the given BCL 
    if this list is not empty:
      for each cube in the given BCL
        with all variables of the group, which are NOT part of used var group
          set the varibale to 01 in the current cube
        

*/

int bcl_ExcludeBCLVars(bcp p, bcl l, bcl grp)
{
  bc l_var;
  bc grp_var;
  bc c;
  int is_any_var_used;
  int i, j;
  int grp_var_cnt = 0;
  __m128i mask;

  
  bcp_StartCubeStackFrame(p);
  l_var = bcp_GetTempCube(p);
  grp_var = bcp_GetTempCube(p);
  
  
  if ( l_var == NULL || grp_var == NULL )
    return bcp_EndCubeStackFrame(p), 0;

  /* get the unate/binate status of both lists */
  bcp_AndElementsBCL(p, l, l_var);
  bcp_AndElementsBCL(p, grp, grp_var);

  //bcp_ShowBCL(p, grp);
  //bcp_ShowBCL(p, l);
  
  /* STEP 1: find out, whether any variables from grp_var are used in l_var */
  is_any_var_used = 0;
  for( i = 0; i < p->var_cnt; i++ )
  {
    if ( bcp_GetCubeVar(p, grp_var, i) != 3 )   // if the variable is in the group, then...
    {
      grp_var_cnt++;
      if ( bcp_GetCubeVar(p, l_var, i) != 3 ) // if the same variable is also used in the BCL
      {
        is_any_var_used = 1;
        break;
      }
    }
  }
  
  if ( is_any_var_used == 0 )           
  {
    logprint(2, "bcl_ExcludeBCLVars: No group variable used (%d vars in the group)", grp_var_cnt);

    return bcp_EndCubeStackFrame(p), 1; // none of the variables of the group are used, so keep the BCL as it is
  }
  logprint(2, "bcl_ExcludeBCLVars: Some group variables used (%d vars in the group)", grp_var_cnt);
    
  /* STEP 2: Calculate "and" mask */
  /* it this point, some variables of the group are used */
  /* goal is now to force variables in BCL to "01", which are previoulsy not used in the BCL ("11") */
  /* for this goal, we will reuse grp_var as a mask for the BCL: */
  /* group vars, which are used in the BCL, will become "11", leaving those variables untouched */
  /* group vars, which are not used in the BCL, will become "01", forcing those vars to zero ("01") */
  for( i = 0; i < p->var_cnt; i++ )
  {
    if ( bcp_GetCubeVar(p, grp_var, i) != 3 )   // if the variable is in the group, then...
    {
      if ( bcp_GetCubeVar(p, l_var, i) != 3 ) // if the same variable is also used in the BCL
      {
        bcp_SetCubeVar(p, grp_var, i, 3);       // the group var is used in BCL, so revert this to DC
      }
      else
      {
        bcp_SetCubeVar(p, grp_var, i, 1);       // the group var is not used in BCL, so force the value to "01"
      }
    }
  }
  
  /* STEP 3: apply "and" mask */
  /* now the mask is inside grp_var and we can just "and" the mask with all cubes in BCL */
  for( j = 0; j < p->blk_cnt; j++ )
  {
    mask = _mm_loadu_si128(grp_var+j);  // grp_var now contains the mask for the BCL cubes
    for( i = 0; i < l->cnt; i++ )
    {
      c = bcp_GetBCLCube(p,l,i);
      _mm_storeu_si128(c+j, _mm_and_si128(_mm_loadu_si128(c+j), mask)); // force the unused vars from the group to zero
    }
  }
  

  bcp_EndCubeStackFrame(p);
  return 1;
}

