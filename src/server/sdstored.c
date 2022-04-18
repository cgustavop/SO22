#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

enum transformations {nop, bcompress, bdecompress, gcompress, gdecompress, encrypt, decrypt};
#define CLIENT_TO_SERVER_FIFO "tmp/client_to_server_fifo" //path para o fifo que permite a comunicação entre o cliente e o servidor

//definir transformações
typedef struct transformation {
    char name[16];
    char bin[16];
    int max_inst;
    int running_inst;
} Transformation;

//definir processos
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

int server_pid; //para guardar o pid do servidor
char transf_folder[1024]; //onde será guardado o path da pasta onde se encontram os executáveis das transformações
Transformation transformation_list[6]; //lista das várias transformações

//definir a queue

void set_config(char* config_file_path){
    //lê o config file linha a linha e define parâmetros para as várias transformações

}

void send_status(){

}



int main(int argc, char* argv[]){

    server_pid = getpid();
    strcpy(transf_folder, argv[2]); //guarda o path

    set_config(argv[1]); //

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