/*
   Zachary Ikpefua
   ECE 3220
   Project 1-2
   sctracer
 */

#define EXIT_CODE_1 231
#define OUTPUT_LOC 2
#define INPUT_LOC 1
#define ADJUST_NULL_SPACE 1
#define ARGS 3
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>


typedef struct list_node_tag {
        int calls;
        int sys_num;
        struct list_node_tag *next;
        struct list_node_tag *prev;
} list_node_t;

typedef struct list_tag {
        list_node_t *head;
        list_node_t *tail;
        int current_list_size;
} list_t;

typedef list_node_t* IteratorPtr;
typedef list_t* ListPtr;

void addSystemCall(int, ListPtr);
ListPtr list_construct(void);
void sortSysCall(ListPtr, IteratorPtr, IteratorPtr);
int comp_proc(int,int);
void sendOutput(FILE*,ListPtr);
void list_destruct(list_t *);


int main(int argc, char **argv) {

        if(argc != ARGS) {
                fprintf(stderr,"usage: ./sctracer arg1 arg2\n");
                exit(1);
        }
        pid_t child = fork();
        FILE *output_file = NULL;
        ListPtr L;
        L = list_construct();
        if (child == 0) {
                //trace the child
                ptrace(PTRACE_TRACEME);
                kill(getpid(), SIGSTOP);
                //take in the inputs from the command line
                char * args[argc + 50];
                const char s[2] = " ";
                char * bstr;
                int i = 1;
                bstr = strtok(argv[INPUT_LOC],s);
                //using argv1 because thats where the test file with multiple inputs is
                while(bstr != NULL) {
                        args[i - ADJUST_NULL_SPACE] = malloc(strlen(argv[INPUT_LOC]) + ADJUST_NULL_SPACE);
                        strcpy(args[i - ADJUST_NULL_SPACE],bstr);
                        bstr = strtok(NULL,s);
                        i++;
                }
                args[i - 1] = malloc(strlen(argv[OUTPUT_LOC]) + ADJUST_NULL_SPACE);
                strcpy(args[i - 1],argv[OUTPUT_LOC]);
                i++;
                args[i - ADJUST_NULL_SPACE] = NULL;
                //execvp the program with argurments
                execvp(args[0],args);
                perror("failed execvp");
                for(i = 0; i < argc; i++) {
                        if(args[i] != NULL) {
                                free(args[i]);
                        }
                }

        } else {
                int status,syscall_num;

                //I'm the parent...keep tabs on that child process
                //wait for the child to stop itself
                waitpid(child, &status, 0);

                //this option makes it easier to distinguish normal traps from
                //system calls
                ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
                //create an infinite loop to capture all system calls
                while(1) {
                        ptrace(PTRACE_SYSCALL, child, 0, 0);    // ignore any signal and continue the child
                        waitpid(child,&status,0);
                        //system call has occured
                        if ((WIFSTOPPED(status) || WSTOPSIG(status) & 0x80)) {
                                syscall_num = ptrace(PTRACE_PEEKUSER, child, sizeof(long)*ORIG_RAX, NULL);
                                //add to linked list
                                addSystemCall(syscall_num, L);
                        }
                        //child has finished
                        if (WIFEXITED(status)) {
                                break;
                        }
                }
                sortSysCall(L,L->head,L->tail);
                //open the output file to write output to it
                output_file = fopen(argv[OUTPUT_LOC],"w+");
                sendOutput(output_file,L);
                fclose(output_file);
                list_destruct(L);
                //let the child finish completely
                ptrace(PTRACE_CONT, child, NULL, NULL);
                waitpid(child, NULL, 0);
        }
}
/*
   FUNCTION: list_construct
   INPUTS: none
   OUTPUTS: list header
   Purpose: Create a linked list by mallocing the header structure
 */
ListPtr list_construct()
{
        list_t *lst;
        lst = (list_t *) malloc(sizeof(list_t));
        lst->head = NULL;
        lst->tail = NULL;
        lst->current_list_size = 0;
        return lst;
}
/*
   FUNCTION: addSystemCall
   INPUTS: system call number, list L
   OUTPUTS: none
   Purpose: Add a system call entry to the linked list
 */
void addSystemCall(int number,ListPtr list_ptr){
        // insert your code here
        IteratorPtr NewNode = (list_node_t*)malloc(sizeof(list_node_t));
        list_node_t *idx_ptr = list_ptr->head; //set rover to head every time
        bool found = false;

        if(list_ptr->head == NULL && list_ptr->tail == NULL) {
                //No elements in the list, set the first and set it to head and tail
                list_ptr->head = NewNode;
                list_ptr->tail = NewNode;
                idx_ptr = list_ptr->head;
                list_ptr->tail = list_ptr->head;
                idx_ptr->prev = NULL;
                idx_ptr->next = NULL;
                idx_ptr->sys_num = number;
                idx_ptr->calls++;
                list_ptr->current_list_size++;
        }
        else{
                while(idx_ptr != NULL) {
                        if(idx_ptr->sys_num == number) {
                                idx_ptr->calls++;
                                found = true;
                                break;
                        }
                        idx_ptr = idx_ptr->next;
                }
                if(!found) {
                        list_ptr->tail->next = NewNode;
                        NewNode->prev = list_ptr->tail;
                        list_ptr->tail = NewNode;
                        NewNode->next= NULL; //last entr
                        NewNode->sys_num = number;
                        NewNode->calls++;
                        list_ptr->current_list_size++;
                }
        }
}
/*
   FUNCTION: sortSysCall
   INPUTS: linked list, head pointer, tail pointer
   OUTPUTS: none
   Purpose: sort the system calls by number
 */
void sortSysCall(ListPtr A, IteratorPtr m, IteratorPtr n)
{
        IteratorPtr MaxNode, i;
        int tempSys, tempCalls;
        while (m != A->tail) {
                i = m;
                MaxNode = m;

                do {
                        i = i->next;
                        if (comp_proc(MaxNode->sys_num, i->sys_num) == -1) MaxNode = i;
                } while (i != n);

                tempSys = m->sys_num;
                m->sys_num = MaxNode->sys_num;
                MaxNode->sys_num = tempSys;
                tempCalls = m->calls;
                m->calls = MaxNode->calls;
                MaxNode->calls= tempCalls;
                m = m->next;
        }
}
/*
   FUNCTION: comp_proc
   INPUTS: two differnt system call number
   OUTPUTS: 1,-1,0 -1 means entry should be moved, 0 and 1 means
          entry should not be moved
   Purpose: Used within the sort system call function to see if the record needs to move
 */
int comp_proc(int record_a, int record_b){
        if (record_a < record_b)
                return 1;
        else if (record_a > record_b)
                return -1;
        else
                return 0;
}
/*
   FUNCTION: sendOutput
   INPUTS: fileptr, listptr
   OUTPUTS: none
   Purpose: Print out the information to the output file listed in the command line argument
 */
void sendOutput(FILE* out, ListPtr list_ptr){
        list_node_t *idx_ptr = list_ptr->head;
        while(idx_ptr != NULL) {
                //dont divide by 2
                if(idx_ptr->sys_num == EXIT_CODE_1) {
                        fprintf(out,"%d\t%d\n",idx_ptr->sys_num,idx_ptr->calls);
                }
                else{
                        //divide all calls by 2 (to cound only the entry call)
                        idx_ptr->calls /= 2;
                        fprintf(out,"%d\t%d\n",idx_ptr->sys_num,idx_ptr->calls);
                }
                idx_ptr = idx_ptr->next;
        }
}
/* Deallocates the contents of the specified list, releasing associated memory
 * resources for other purposes.
 *
 * Free all elements in the list and the header block.
    Input: list_ptr
 */

void list_destruct(list_t *list_ptr)
{
        if(list_ptr == NULL) {
                free(list_ptr);
        }
        else if(list_ptr->head == NULL && list_ptr->tail == NULL) {
                free(list_ptr);
                list_ptr = NULL;
        }
        // the first line must validate the list
        else {
                list_node_t *junk = list_ptr->tail;
                list_node_t *rover = list_ptr->tail->prev;
                while(rover != NULL) {
                        junk->next = NULL;
                        junk->prev = NULL;
                        rover->next = NULL;
                        free(junk);
                        junk = rover;
                        rover = rover->prev;
                }
                free(junk);
                junk = NULL;
                list_ptr->tail = NULL;
                list_ptr->head = NULL;
                list_ptr->current_list_size = 0;
                free(list_ptr);
                list_ptr = NULL;
        }
}
