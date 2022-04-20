#ifndef __PROCESS_H__
#define __PROCESS_H__


typedef struct process {
    int client_pid;
    int fork_pid;
    char transf_names[16][16];
    char input[1024];
    char output[1024];
    int number_transformations;
    int active;
    int inqueue;
} Process;

#endif