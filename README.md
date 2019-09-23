Zachary Ikpefua - Clemson ECE 3220
# Project 1: Tracing, System Calls, and Processes

KNOWN PROBLEMS:
none found

DESIGN:
leakcount - created a memory_shim file that houses all shimed library functions.
malloc will take in the users request for mallocing and place the data in a linked list for storing
free will take the users request and free the data and look for the data in the linked list and change the status to "freed"

sctracer- program that will scan the system calls and place them in an linked list which will then sort the linked list
and finally output to a output file
-The while loop is infinite so that it takes all the system calls until the execvp program is finished
