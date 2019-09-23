#define _GNU_SOURCE

void __attribute__ ((constructor)) start(void);
void __attribute__ ((destructor)) finish(void);
/*
   Zachary Ikpefua
   ECE 3220
   memory_shim.c
   Project 1-1
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <dlfcn.h>
 #include <stdbool.h>


typedef void * EmptyPtr;

typedef struct list_node_tag {
        // private members for list.c only
        bool freed;
        EmptyPtr data;
        long unsigned bytes;
        struct list_node_tag *next;
} list_node_t;

typedef struct list_tag {
        // private members for list.c only
        list_node_t *head;
        list_node_t *tail;
        int current_list_size;
} list_t;


typedef list_node_t * IteratorPtr;
typedef list_t * ListPtr;

void *(*original_malloc)(size_t size) = NULL;
void (*original_free)(EmptyPtr ptr) = NULL;
//declare global linked list
ListPtr L;

void start(void){
        //new malloc and free
        original_free = dlsym(RTLD_NEXT, "free");
        original_malloc = dlsym(RTLD_NEXT,"malloc");
        //create the linked list
        L = (ListPtr) original_malloc(sizeof(list_t));
        L->head = NULL;
        L->tail = NULL;
        L->current_list_size = 0;
}

EmptyPtr malloc(size_t size){
        //add to linked list to check if it is freed then return the original malloc to act like nothing happened
        IteratorPtr new = (IteratorPtr)original_malloc(sizeof(list_node_t));
        EmptyPtr p = original_malloc(size);
        if(L->current_list_size == 0) {
                //create the first node
                L->head = new;
                L->tail = new;
                new->next = NULL;
                new->freed = false;
                new->bytes = size;
                new->data = p;
                L->current_list_size++;
        }
        else{
                //use the tail and update list like queue
                L->tail->next = new;
                new->next = NULL;
                new->freed = false;
                new->data = p;
                new->bytes = size;
                L->tail = new;
                L->current_list_size++;
        }
        //stored in linked list, return the malloced pointer to user
        return p;
}

void free(EmptyPtr ptr){
        IteratorPtr idx = L->head;
        if(L->current_list_size != 0 && ptr != NULL) {
                //free is looking for the same data ptr called in function
                while (idx != NULL) {
                        if(idx->data == ptr && idx->freed == false) {
                                idx->freed = true;
                                break;
                        }
                        idx = idx->next;
                }
                original_free(ptr);
        }
}

void finish(void){
        long unsigned leakcount = 0;
        long unsigned totbytes = 0;
        IteratorPtr idx = L->head;
        while (idx != NULL) {
                //print the leak information if it was not freed
                if(idx->freed == false) {
                        fprintf(stderr,"LEAK\t%lu\n",idx->bytes);
                        leakcount++;
                        totbytes += idx->bytes;
                }
                idx = idx->next;
        }
        fprintf(stderr,"TOTAL\t%lu\t%lu\n",leakcount,totbytes);

        //destruct linked list
        IteratorPtr rover = L->head;
        idx = L->head;
        while(idx != NULL) {
                rover = rover->next;
                idx->next = NULL;
                idx->data = NULL;
                free(idx);
                idx = rover;
        }
        L->head = NULL;
        L->tail = NULL;
        original_free(L);
        L = NULL;
}
