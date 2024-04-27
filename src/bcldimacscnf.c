/*

  bcldimacscnf.c
  
  Support for the dimacs cnf file format

  Boolean Cube Calculator
  (c) 2024 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/

*/

#include "bc.h"
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <assert.h>

int fp_current_char = '\0';

void fp_skip_space(FILE *fp)
{
  for(;;)
  {
    if ( fp_current_char < 0 )
      break;
    if ( fp_current_char > 32 )
      break;
    fp_current_char = fgetc(fp);
  }
}

const char *fp_get_identifier(FILE *fp)
{
  static char identifier[1024];
  int i = 0;
  identifier[0] = '\0';
  if ( isalpha(fp_current_char) || fp_current_char == '_' )
  {
    for(;;)
    {
      if ( fp_current_char < 0 )
        break;
      if ( isalnum(fp_current_char) || fp_current_char == '_' )
      {
        if ( i+1 < 1024 )
          identifier[i++] = fp_current_char;
        fp_current_char = fgetc(fp);
      }
      else
      {
        break;
      }
    }
  }
  identifier[i] = '\0';
  fp_skip_space(fp);
  return identifier;
}



int fp_get_value(FILE *fp)
{
  int v = 0;
  int is_minus = 0;
  if ( fp_current_char == '-' )
  {
    is_minus = 1;
    fp_current_char = fgetc(fp);
    fp_skip_space(fp);
  }
  if ( isdigit(fp_current_char) )
  {
    for(;;)
    {
      if ( fp_current_char < 0 )
        break;
      if ( isdigit(fp_current_char) )
      {
        v = (v*10) + (fp_current_char) - '0';
        fp_current_char = fgetc(fp);
      }
      else
      {
        break;
      }
    } // for
  }
  fp_skip_space(fp);
  if ( is_minus )
    return -v;
  return v;
}


int bcp_GetVarCntFromDIMACSCNF(FILE *fp, int *var_cnt, int *clause_cnt)
{
  const char *subformat;
  fseek(fp, 0, SEEK_SET);
  fp_current_char = fgetc(fp);
  fp_skip_space(fp);
  
  for(;;)
  {
    if ( fp_current_char == 'c' ||   fp_current_char == 'C' )
    {
      for(;;)
      {
        fp_current_char = fgetc(fp);
        if ( fp_current_char < 0 )
          return 0;
        if ( fp_current_char == '\r' || fp_current_char == '\n' )
          break;
      }
      fp_skip_space(fp);
    }
    else if ( fp_current_char == 'p' || fp_current_char == 'P' )
    {
      // p cnf 60 160
      
      fp_current_char = fgetc(fp);
      fp_skip_space(fp);
      subformat = fp_get_identifier(fp);
      if ( strcasecmp(subformat, "cnf") != 0 )
        return 0;       // only "cnf" is supported
      *var_cnt = fp_get_value(fp);
      *clause_cnt = fp_get_value(fp);
      return 1;
    }
    else
      fp_current_char = fgetc(fp);
  }
  
}


int bcp_AddBCLCubesByDIMACSCNF(bcp p, bcl l, FILE *fp)
{
  fseek(fp, 0, SEEK_SET);
  fp_current_char = fgetc(fp);
  fp_skip_space(fp);
  
  for(;;)
  {
    if ( fp_current_char < 0 )
        return 1;
    if ( fp_current_char == 'c' ||   fp_current_char == 'C' || fp_current_char == 'p' || fp_current_char == 'P')
    {
      for(;;)
      {
        fp_current_char = fgetc(fp);
        if ( fp_current_char < 0 )
          return 1;
        if ( fp_current_char == '\r' || fp_current_char == '\n' )
          break;
      }
      fp_skip_space(fp);
    }
    else
    {
      if ( isdigit( fp_current_char ) || fp_current_char == '-' )
      {
        int pos = bcp_AddBCLCube(p, l); // add empty cube to list l, returns the position of the new cube or -1 in case of error
        if ( pos < 0 )
        {
          return 0;
        }
        else
        {
          int var = 0;
          bc c = bcp_GetBCLCube(p, l, pos);
          
          for(;;)
          {
            var = fp_get_value(fp);
            if ( var == 0 )     // the value 0 terminates the clause
              break;
            if ( var < 0 )
            {
              bcp_SetCubeVar(p, c, -var-1, 2);     // invert the minterm and assign "one" 
            }
            else
            {
              bcp_SetCubeVar(p, c, var-1, 1);     // assign zero
            }           
            if ( fp_current_char < 0 )
              return 1;
            
          } // for(;;)
        } // add cube        
      } // isdigit
      else
      {
        fp_current_char = fgetc(fp);
        fp_skip_space(fp);
        
      }
    }
  } // for(;;)
}


bcp bcp_NewByDIMACSCNF(FILE *fp)
{
  int var_cnt = 0;
  int clause_cnt = 0;
  if ( bcp_GetVarCntFromDIMACSCNF(fp, &var_cnt, &clause_cnt) == 0 )
    return NULL;
  
  return bcp_New(var_cnt);
}

bcl bcp_NewBCLByDIMACSCNF(bcp p, FILE *fp)   // create a bcl from a DIMACS CNF
{
  int var_cnt = 0;
  int clause_cnt = 0;
  bcl l = bcp_NewBCL(p);
  if ( l == NULL )
    return NULL;
  if ( bcp_GetVarCntFromDIMACSCNF(fp, &var_cnt, &clause_cnt) == 0 )
    return bcp_DeleteBCL(p, l), NULL;
  assert( p->var_cnt == var_cnt);
  if ( bcp_AddBCLCubesByDIMACSCNF(p, l, fp) == 0 )
    return bcp_DeleteBCL(p, l), NULL;
  assert( l->cnt == clause_cnt );
  return l;
}



