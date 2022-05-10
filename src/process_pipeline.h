#ifndef _PROCESS_PIPELINE_H__
#define _PROCESS_PIPELINE_H__

#include <stdlib.h>
#include <stdbool.h>

typedef struct processPipeline ProcessPipeline;

// Makes a pipeline that will read from input_fd and write to output_fd, passing the data
// through added processes
ProcessPipeline *pp_new(int process_num, int input_fd, int output_fd);

// sends SIGTERM to children then frees struct
void pp_free(ProcessPipeline *pp);

// Forks and adds a process to the pipeline
// returns false if pipeline is full
bool pp_add(const char *exec_path, char *const exec_args[], ProcessPipeline *pp);

// Waits for every process to finish and returns false if a child returned non zero
bool pp_wait_check_all(ProcessPipeline *pp);

// Returns number of finished processes or -1 if child returned non zero
int pp_check_end_num(ProcessPipeline *pp);

int pp_get_len(ProcessPipeline *pp);


#endif