#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


                                    //    0      1           2               3                   4...
int main(int argc, char* argv[]){ //./sdstore proc-file samples/file-a outputs/file-a-output bcompress nop gcompress encrypt nop

    int i;

    if(argc >= 5){ //5 argumentos é o mínimo para se ter um executável na chamada
        if(strcmp(argv[1], "proc-file") == 0){
                int fd_input = open(argv[2], O_RDONLY); //abre o input
                int fd_output = open(argv[3], O_RDONLY);    //abre o ficheiro de destino

                if(fd_input && fd_output){ //se não houve problema na abertura de ficheiros
                    for(i = 4; i <= argc; i++){ //percorre os argumentos com os executáveis
                        request(argv[i], fd_input, fd_output); //request() envia uma request ao servidor passando os ficheiros e os executáveis 
                    }
                }else{
                    printf("Invalid file path.")
                }


        }
    }
    return 0;
}