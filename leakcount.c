#define _GNU_SOURCE
/*
   Zachary Ikpefua
   ECE 3220
   Project 1-1 Leakcount

   Purpose:  Count all the leaks in a given program from the command line
 */
#define ADJUST_NULL_SPACE 1 //for the null needed on execvp
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char** argv) {
        int i = 0;
        //add my shim library
        setenv("LD_PRELOAD","./memory_shim.so",1);
        if(fork() == 0) {
                char * args[argc + ADJUST_NULL_SPACE];
                for(i = 1; i < argc; i++) {
                        //argv arguments start at 1
                        args[i - 1] = malloc(strlen(argv[i]) + ADJUST_NULL_SPACE);
                        strcpy(args[i-ADJUST_NULL_SPACE],argv[i]);
                }
                args[i-ADJUST_NULL_SPACE] = NULL;
                execvp(args[0],args);
                //if execvp fails
                perror("failed execvp");
                //free the args array
                for(i = 0; i < argc; i++) {
                        if(args[i] != NULL) {
                                free(args[i]);
                        }
                }
        }
        //wait for the child
        wait(NULL);
        return 0;
}
