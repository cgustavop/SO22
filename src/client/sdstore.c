#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define CLIENT_TO_SERVER_FIFO "tmp/client_to_server_fifo" //path para o fifo que permite a comunicação entre o cliente e o servidor

int status(){
    //listar tasks
        //for loop
    return 0; //return 1 se consegue estabelecer ligação com o servidor e 0 caso contrário
}

//recebe a lista dos vários executáveis e os paths de input e output dos ficheiros para enviar uma request ao servidor
int request(char** execs, char* ipath, char* opath){
    //escreve num FIFO
    return 0; //return 1 se consegue estabelecer ligação com o servidor e 0 caso contrário
}
                                    //    0      1           2               3                   4...
int main(int argc, char* argv[]){ //./sdstore proc-file samples/file-a outputs/file-a-output bcompress nop gcompress encrypt nop

    int i, j = 0;

    if(argc > 1){
        if(strcmp(argv[1], "proc-file") == 0){
            
            char** execs[argc - 4];

            for(i = 4; i < argc; i++){ //percorre os argumentos com os executáveis
                execs[j] = argv[i];
                j++;
            }
            
            //request() envia uma request ao servidor passando os nomes dos executaveis e os paths dos ficheiros
            if(!request(execs, argv[2], argv[3])) printf("Não foi possível fazer request ao servidor.");

        } else if(strcmp(argv[1], "status") == 0){
                if(!status()) printf("Não foi possível estabelecer ligação ao servidor.");
        }
    }
    return 0;
}