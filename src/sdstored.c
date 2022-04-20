#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "process.h"

enum transformations {nop, bcompress, bdecompress, gcompress, gdecompress, encrypt, decrypt};
#define CLIENT_TO_SERVER_FIFO "tmp/client_to_server_fifo" //path para o fifo que permite a comunicação entre o cliente e o servidor
#define TOTAL_TRANSFORMATIONS_NUMBER 6
//definir transformações
typedef struct transformation {
    char name[16];
    //char bin[16]; not sure se isto é necessário na realidade
    int max_inst;
    int running_inst;
} Transformation;

/*definir processos
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
*/

int server_pid; //para guardar o pid do servidor
char transf_folder[1024]; //onde será guardado o path da pasta onde se encontram os executáveis das transformações
Transformation transfs[TOTAL_TRANSFORMATIONS_NUMBER]; //lista das várias transformações

//definir a queue para 20 users em simultâneo



int readc(int fd, char* c){

    return read(fd, c, 1);
}

ssize_t readln(int fd, char *buf, size_t size){
    
    int i = 0;

    while(i < size && readc(fd, &buf[i]) > 0){
        i++;
        if(((char*) buf)[i-1] == '\n'){
            return i;
        }
    }

    return i;
}

// <exec file name> <max. paralel instances>
int set_config(char* config_file_path){ //lê o config file linha a linha e define parâmetros para as várias transformações
    
    int fd = openat(AT_FDCWD, config_file_path, O_RDONLY);
    printf("%s\n", config_file_path);
    if(fd == -1) return 1;
    char buf[64];

    while ((readln(fd, buf, 64)) > 0) {
        
        char* token = strtok(buf, " ");
        int i, j = 0;
        for (i = 0; i < 2 && token != NULL; i++) {
            switch (i) {
                case 0:
                    //printf("name: %s", token);
                    strncpy(transfs[j].name, token, 15);
                    //strcpy(transfs[j].bin, token);
                    break;
                case 1:
                    //strcpy(transfs[j].max_inst, token);
                    transfs[j].max_inst = atoi(token);
                    //printf("\n\tmax instances: %d\n", transfs[j].max_inst);
                    break;
            }
            token = strtok(NULL, " "); // cleaning token
        }
        transfs[j].running_inst = 0;
        j++;
    }
    close(fd);

    return 0;
}

void send_status(){

}

void read_from_pipe(){

}

void write_to_pipe(){

}

void apply_transformation(){

}

void handle_request(int fd){
    
    //comunicação do cliente->servidor
    Process process;

    while(1){
        sleep(1);
        if(read(fd, &process, sizeof(struct process)) > 0){;

            //debug
            printf("Request details:\n");
            printf("\tClient PID: %d\n", process.client_pid);
            printf("\tFirst transformation: %s\n", process.transf_names[0]);
            printf("\tInput path: %s\n", process.input);
            printf("\tOutput path: %s\n", process.output);
            printf("\tTotal transformations requested: %d\n", process.number_transformations);

        }else printf("Nothing to read.\n");
    }
    
}
int main(int argc, char* argv[]){

    server_pid = getpid();
    strcpy(transf_folder, argv[1]); //guarda o path onde estão guardadas as transformações
    printf("SETTING UP SERVER...\n");
    if(set_config(argv[2])){ //se retornou um valor != 0 ocorreu algum erro
        printf("Erro na leitura do ficheiro %s\n", argv[2]);
        return 1;
    }

    printf("Opening client to server fifo...\n");
    mkfifo(CLIENT_TO_SERVER_FIFO, 0666);
    int fd = open(CLIENT_TO_SERVER_FIFO, O_RDONLY | O_NONBLOCK);
    if(fd == -1){ 
        printf("Failed opening FIFO.\n");
    }else printf("FIFO created.\n");

    printf("\nSETUP COMPLETE...\n");

    printf("Searching for requests...\n");
    handle_request(fd);

    return 0;
}

/*  EXEMPLO ENVIO DE STATUS

task #3: proc-file 0 /home/user/samples/file-c file-c-output nop bcompress
task #5: proc-file 1 samples/file-a file-a-output bcompress nop gcompress encrypt nop
task #8: proc-file 1 file-b-output path/to/dir/new-file-b decrypt gdecompress
transf nop: 3/3 (running/max)
transf bcompress: 2/4 (running/max)
transf bdecompress: 1/4 (running/max)
transf gcompress: 1/2 (running/max)
transf gdecompress: 1/2 (running/max)
transf encrypt: 1/2 (running/max)
transf decrypt: 1/2 (running/max)
*/  