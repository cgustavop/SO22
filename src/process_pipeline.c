#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <wait.h>
#include <assert.h>



typedef struct processPipeline{
    int pid_arr_len;
    int pid_arr_size;
    int end_num;
    int reader_pipe;
    int input_fd, output_fd;
    int pid_arr[];
}ProcessPipeline;


ProcessPipeline *pp_new(int process_num, int input_fd, int output_fd){
    ProcessPipeline *pp = malloc(sizeof(ProcessPipeline)+process_num*sizeof(int));
    pp->pid_arr_size = process_num;
    pp->pid_arr_len = 0;
    pp->end_num = 0;
    pp->input_fd = input_fd;
    pp->output_fd = output_fd;

    return pp;
}

void pp_free(ProcessPipeline *pp){
    for(int i=0;i<pp->pid_arr_len;++i)
        kill(pp->pid_arr[i], SIGTERM);
    free(pp);
}

void pp_term_processes(ProcessPipeline *pp){
    for(int i=0;i<pp->pid_arr_len;++i)
        kill(pp->pid_arr[i], SIGTERM);
}

bool pp_add(const char *exec_path, char *const exec_args[], ProcessPipeline *pp){
    
    if(pp->pid_arr_len==pp->pid_arr_size)
        return false;

    int pipefd[2], pid;

    if(pipe(pipefd)==-1)
        return false;

    if((pid=fork())==0){
        close(pipefd[0]);

        if(pp->pid_arr_len==0){
            if(pp->input_fd!=STDIN_FILENO){
                dup2(pp->input_fd, STDIN_FILENO);
                close(pp->input_fd);
            }
        }
        else{
            dup2(pp->reader_pipe, STDIN_FILENO);
            close(pp->reader_pipe);
        }

        if(pp->pid_arr_len+1==pp->pid_arr_size){
            if(pp->output_fd!=STDOUT_FILENO){
                dup2(pp->output_fd, STDOUT_FILENO);
                close(pp->output_fd);
            }
        }
        else{
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
        }

        execvp(exec_path, exec_args);

        perror("Failed to start Process Pipeline child");
        _exit(1);
    }

    close(pipefd[1]);
    if(pp->pid_arr_len!=0)
        close(pp->reader_pipe);
    pp->reader_pipe = pipefd[0];

    pp->pid_arr[pp->pid_arr_len] = pid;
    ++pp->pid_arr_len;

    if(pp->pid_arr_len == pp->pid_arr_size)
        close(pipefd[0]);

    return true;
}

bool pp_wait_check_all(ProcessPipeline *pp){
    for(int i=0;i<pp->pid_arr_len;++i){
        int st;
        waitpid(pp->pid_arr[i], &st, 0);
        if(!WIFEXITED(st) || WEXITSTATUS(st)!=0)
            return false;
    }
    return true;
}

int pp_check_end_num(ProcessPipeline *pp){
    while (pp->end_num < pp->pid_arr_len){
        int st;
        if(waitpid(pp->pid_arr[pp->end_num],&st, WNOHANG)==0)
            break;
        else{
            ++pp->end_num;
            if(WEXITSTATUS(st)!=0)
                return -1;
        }
    }
    return pp->end_num;
}

int pp_get_len(ProcessPipeline *pp){
    return pp->pid_arr_len;
}

//grep -v ˆ# /etc/passwd | cut -f7 -d: | uniq | wc -l


// int main(int argc, char *argv[]){
    
//     ProcessPipeline *pp = pp_new(4, STDIN_FILENO, STDOUT_FILENO);


//     assert(pp_add("grep", (char*[]){"grep", "-v", "^#", "/etc/passwd", 0}, pp));

//     assert(pp_add("cut", (char*[]){"cut", "-f7", "-d:", 0}, pp));

//     assert(pp_add("uniq", (char*[]){"uniq", 0}, pp));

//     assert(pp_add("wc", (char*[]){"wc", "-l", 0}, pp));

//     // printf("exit:%d\n", pp_wait_check_all(pp));

//     size_t pp_num = pp_get_process_num(pp), n=0;
//     while((n=pp_check_end_num(pp))<pp_num);
//     printf("end num:%ld\n",n);
//     pp_free(pp);
    

//     return 0;
// }