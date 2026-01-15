# boolean cube

## License

Boolean Cube Calculator, Copyright 2024 by Oliver Kraus

https://creativecommons.org/licenses/by-sa/4.0/deed.en

## Terms

This project deals with lists of boolean cubes. A boolean cube is a conjuction of variables. A list of such cubes
represents a disjunction. The following terms are interchangeable and refer to the same object:

 - Boolean Cube List (BCL)
 - Sum of Porducts (SOP, https://en.wikipedia.org/wiki/Canonical_normal_form)
 - Disjunctive Normal Form (DNF, https://en.wikipedia.org/wiki/Disjunctive_normal_form)
 
In this project we will use the term BCL, which usually referes to a SOP / DNF expression.

## Theoretical Background

Most of the algorithms used in this project are summarized in the technical report "Multiple-Valued Logic Minimization for PLA Synthesis"
from Richard L. Rudell, see https://www2.eecs.berkeley.edu/Pubs/TechRpts/1986/734.html 

The project owner is author of the book "Synthese von digitalen asynchronen Zustandsautomaten" (ISBN 9783183372201), which contains
a detailed description of all the algorithms used in this project.

Other references include:

Giovanni DeMicheli,
"Synthesis and Optimization of Digital Circuits", 1994,
McGraw-Hill Book Company, Inc.,
ISBN 0-07-113271-6
  
Robert K. Brayton, Gary D. Hachtel, Curtis T. McMullen and 
Alberto L. Sangiovanni-Vincentelli,
"Logic Minimization Algorithms for VLSI Synthesis", 1994,
Kluwer Academic Publishers Group,
ISBN 0-8983-164-9

Note: This project limits the calculation to boolean logic only. Multi-valued logic is not part of this project.

## Purpose

This project includes the tool "boolean cube calculator" (bcc).
Main features:

 - Input and output in JSON format
 - Convertion of boolean expressions into the internal BCL object
 - High level transformation and calculation on the BCL object:
	- Minimization
	- Complement
	- Union
	- Intersection
	- Super- and Subset Test
	- Equality Test
	
## Implementation

This project is a re-implementation of my older project DGC (https://github.com/olikraus/dgc and https://sourceforge.net/projects/dgc/files/dgc/) 

This project requires SSE2.0 and will also make use of the "__builtin_popcountll" command of the gcc compiler. 
For the "__builtin_popcountll" command it is suggested to enable a machine architecture which maps  "__builtin_popcountll" to the corrsponding processor instruction (https://en.wikipedia.org/wiki/X86_Bit_manipulation_instruction_set).

The minimum required processor will be a "Pentium 4" (https://en.wikipedia.org/wiki/Pentium_4): `-march=pentium4`.
To allow fast popcnt implementation use `-march=silvermont`.
More details can be found here: https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html (bcc requires SSE2 and runs faster with POPCNT).

On a linux pc, use `lscpu` to check for SSE2. The reference guide for SSE is available here: https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html


This project requires the c-object library https://github.com/olikraus/c-object .
Especially the two files "co.h" and "co.c" from https://github.com/olikraus/c-object/tree/main/co are required.

## Command Line Options

 - `-h` Show command line options
 - `-test` Execute internal test procedure. Requires debug version of this executable.
 - `-ojpp` Pretty print JSON output for the next '-json' command.
 - `-ojson <json file>` Provide filename for the JSON output. `stdout` will be used if the output file is not set.
 - `-json <json file>` Parse and execute commands from JSON input file. Multiple `-json` commands are allowed.

The `-json` command will read the json data from the provided argument. Results are written to the specified output file (`-ojson`),
considering the pretty print option (`-ojpp`). Multiple `-json` will be combined and executed as a single list.

A call might look like this:
```
./bcc -ojpp -ojson output.json -json input.json
```

Multiple input/output combinations can be executed:
```
./bcc -json input1.json -json input2.json -ojson output.json
```

Example:
```
./bcc -ojpp -json ../json/minimize.json  -json ../json/dc_var.json
```


## JSON input file

The input JSON is an array of multiple command blocks (described as a JSON map):
```
JSON Input := [ <block1>, <block2>, ..., <blockn>  ]
```

Such a block looks like this:
``` json
{
  "cmd":"<command name>",
  "expr":"<boolean expression>",
  "slot":9,
  "label":"<key>",  
  "label0":"<key>"  
}
```

The command name must be one of the following strings:
 - "bcl2slot": Convert the expression into a BCL and store the result in the given slot.
 - "show": Print the given slot.
 - "minimize": Minimize the content of the given slot.
 - "complement": Calculate the complement of the given slot and overwrite the slot with the result.
 - "intersection0": Calculate the intersection between slot 0 and the given slot. Store the result in slot 0.
 - "union0": Calculate the union between slot 0 and the given slot. Store the result in slot 0.
 - "subtract0": Subtract the given slot from slot 0 and store the result in slot 0.
 - "equal0": Compare the given slot with slot 0 and set several flags accordingly. Store the result in the result json.
 - "exchange0": Exchange the given slot with slot 0.
 - "copy0to": Copy the BCL from slot 0 to the given slot.
 - "copy0from": Copy the BCL from the given slot n to slot 0.
 
The "expr" JSON member is used by the "bcl2slot" command.
The syntax for the boolean expression includes "&" for AND,"|" for OR and "-" for NOT, however such syntax
can be redefined (see  https://github.com/olikraus/boolean_cube/blob/main/json/redef_expr.json).
Expressions can be nested with parenthesis. Any C-like identifier are accepted as variable names.
An expression may look like this: "(a&-c)|(a&b)". 


The "slot" is used by most of the commands as an argument. Many commands will use BCL content of slot 0 
and the BCL provided by the "slot" command. There are ten slots from 0 to 9.

The "label" and "label0" JSON member will generate a JSON map in the output JSON.
 - "label" will output the content of several result flags.
 - "label0" will additionally output the content of slot 0.
 
## JSON output file

The json output file is a map, which includes a map for each "label" or "label0" member found in the input JSON:
``` json
{ 
	"<key>": {
	  "index":99,
	  "empty":0,
	  "subset":0,
	  "superset":0,
	  "expr":"<boolean expression>"
	}
	...
}
```

The argument of a "label" or "label0" member in the JSON input is used as a key for the outer map in the JSON output.
The "expr" member contains the boolean expression of slot 0 if "label0" had been used.
The flags are:

 - "empty":	1 if slot 0 is empty
 - "subset":  1 if slot 0 is subset of/equal with the given slot n for "equal0" cmd
 - "superset":  1 if slot 0 is superset of/equal with the given slot n for "equal0" cmd

Slot 0 content is equal to slot n content if subset and superset are both set to 1.
 
 
 







