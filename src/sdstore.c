#include <assert.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "request.h"

static_assert(sizeof(Request)+sizeof(ProcessRequestData)<=PIPE_BUF, "requests too big, non atomic writes");

int fd[2]; //guarda os file descriptors dos fifos escrita (0), leitura (1)

int status(){
    //listar tasks
        //for loop
    return 0; //return 1 se consegue estabelecer ligação com o servidor e 0 caso contrário
}


int send_process_request(Request *request){
    
    //comunicação do cliente->servidor
    printf("Opening client to server fifo...\n");
    fd[0] = open(CLIENT_TO_SERVER_FIFO, O_WRONLY);
    if(fd[0] == -1) return 1; //falhou ligação
    printf("Connection established\n");
    
    //comunicação do servidor->cliente
    //mkfifo("tmp/server_to_client_fifo", O_RDONLY); 
    //fd[1] = open("tmp/server_to_client_fifo", O_RDONLY);
    printf("Sending request\n");
    write(fd[0], request, sizeof(Request)+sizeof(ProcessRequestData));
    printf("Request sent.\n");

    close(fd[0]);
    return 0; 
}

void create_request(char** execs, int total_num_transfs, char* input_path, char* output_path, Request *req){
    ProcessRequestData *req_data = (ProcessRequestData*)req->data;

    req->client_pid = getpid();
    req->priority = 0;
    req->type = PROCESS_REQUEST;

    for(int i = 0; i < total_num_transfs; i++){
        strncpy(req_data->transf_names[i], execs[i], 16);
    }
    char *inp = realpath(input_path, NULL);
    char *outp = realpath(output_path, NULL);
    strncpy(req_data->input, inp, strlen(inp)+1);
    strncpy(req_data->output, outp, strlen(outp)+1);
    free(inp);
    free(outp);

    req_data->transf_num = total_num_transfs;

}


                                    //    0      1           2               3                   4...
int main(int argc, char* argv[]){ //./sdstore proc-file samples/file-a outputs/file-a-output bcompress nop gcompress encrypt nop

    char req_buf[sizeof(Request)+sizeof(ProcessRequestData)];
    Request *req = (Request*)req_buf;

    int i, total_num_transfs = 0;

    if(argc > 1){
        if(strcmp(argv[1], "proc-file") == 0){
            
            char buf[64];
            int fd=-1, n = snprintf(buf, 64, SERVER_TO_CLIENT_FIFO_TEMPL"%d", getpid());
            if(n<=64 && n>0){
                mkfifo(buf, 0666);
            }

            char* execs[argc - 4];

            for(i = 4; i < argc; i++){
                execs[total_num_transfs] = argv[i];
                total_num_transfs++;
            }

            int fdinp, fdout;
            // realpath falha para ficheiros não existentes
            if( (fdinp=open(argv[2], O_CREAT | O_RDONLY, 0666))==-1 ||
                (fdout=open(argv[3], O_CREAT | O_TRUNC, 0666))==-1)
            {
                perror("Error opening paths");
                return 1;
            }
            else{
                close(fdinp);
                close(fdout);
            }
            
            create_request(execs, total_num_transfs, argv[2], argv[3], req);

            if(send_process_request(req)) perror("Unable to make request.");

            fd = open(buf, O_RDONLY, 0666);
            close(fd);

        } else if(strcmp(argv[1], "status") == 0){
                if(!status()) printf("Não foi possível estabelecer ligação ao servidor.\n");
        }
    }else{
        fprintf(stderr, "wrong arg count\n");
    }
    return 0;
}