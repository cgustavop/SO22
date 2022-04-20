#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "process.h"

#define CLIENT_TO_SERVER_FIFO "tmp/client_to_server_fifo" //path para o fifo que permite a comunicação entre o cliente e o servidor
int fd[2]; //guarda os file descriptors dos fifos escrita (0), leitura (1)

int status(){
    //listar tasks
        //for loop
    return 0; //return 1 se consegue estabelecer ligação com o servidor e 0 caso contrário
}

int send_request(Process process){
    
    //comunicação do cliente->servidor
    printf("Opening client to server fifo...\n");
    fd[0] = open("tmp/client_to_server_fifo", O_WRONLY);
    if(fd[0] == -1) return 1; //falhou ligação
    printf("Connection established\n");
    
    //comunicação do servidor->cliente
    //mkfifo("tmp/server_to_client_fifo", O_RDONLY); 
    //fd[1] = open("tmp/server_to_client_fifo", O_RDONLY);
    printf("Sending request\n");
    write(fd[0], &process, sizeof(struct process));
    printf("Request sent.\n");

    return 0; 
}

Process create_process(char** execs, int total_num_transfs, char* input_path, char* output_path){
    Process process;

    process.client_pid = getpid();
    process.fork_pid = 0;

    for(int i = 0; i < total_num_transfs; i++){
        strncpy(process.transf_names[i], execs[i], 16);
    }

    strncpy(process.input, input_path, 1023);
    strncpy(process.output, output_path, 1023);

    process.number_transformations = total_num_transfs;
    process.active = 0;
    process.inqueue = 0;

    return process;
}
                                    //    0      1           2               3                   4...
int main(int argc, char* argv[]){ //./sdstore proc-file samples/file-a outputs/file-a-output bcompress nop gcompress encrypt nop

    int i, total_num_transfs = 0;

    if(argc > 1){
        if(strcmp(argv[1], "proc-file") == 0){
            
            char* execs[argc - 4];

            for(i = 4; i < argc; i++){
                execs[total_num_transfs] = argv[i];
                total_num_transfs++;
            }
            
            Process process = create_process(execs, total_num_transfs, argv[2], argv[3]);

            if(send_request(process)) printf("Unable to make request.");

        } else if(strcmp(argv[1], "status") == 0){
                if(!status()) printf("Não foi possível estabelecer ligação ao servidor.");
        }
    }
    return 0;
}