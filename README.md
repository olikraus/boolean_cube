# boolean cube

## License

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

The project owner is also author of the book "Synthese von digitalen asynchronen Zustandsautomaten" (ISBN 9783183372201), which also contains
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

Note: This project limits the calculation to boolean logic. Multi-valued logic is not part of this project.

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

This project requires the c-object library https://github.com/olikraus/c-object .
Especially the two files "co.h" and "co.c" from https://github.com/olikraus/c-object/tree/main/co are required.

## Command Line Options

 - `-h` Show command line options
 - `-test` Execute internal test procedure. Requires debug version of this executable.
 - `-ojpp` Pretty print JSON output for the next '-json' command.
 - `-ojson <json file>` Provide filename for the JSON output of the next '-json' command.
 - `-json <json file>` Parse and execute commands from JSON input file.

The `-json` command will read the json data from the provided argument. Results are written to the previously specified output file (`-ojson`),
considering the pretty print option (`-ojpp`). As a consequence it is required to specifiy `-ojson` (and `-ojpp`) before the `-json` argument.
A call might look like this:
```
./bcc -ojpp -ojson output.json -json input.json
```

Multiple input/output combinations can be executed:
```
./bcc -ojson output1.json -json input1.json -ojson output2.json -json input2.json
```


