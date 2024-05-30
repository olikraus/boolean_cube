# boolean cube

## License

https://creativecommons.org/licenses/by-sa/4.0/deed.en

## Terms

This project deals with lits of boolean cubes. A boolean cube is a conjuction of variables. A list of such cubes
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
	

