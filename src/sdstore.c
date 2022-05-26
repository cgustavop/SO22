#include <assert.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include "request.h"

#define MAX_BUFFER 4096

static_assert(sizeof(Request)+sizeof(ProcessRequestData)<=PIPE_BUF, "Request's too big. Non atomic writes");

char SERVER_TO_CLIENT_FIFO[25];

 /* Guarda os file descriptors dos FIFOs.
    fd[0] para escrita
    fd[1] para leitura */
int fd[2];

char server_to_client_fifo[84];
 /* Abre o FIFO criado pelo servidor em modo escrita para poder enviar requests para o servidor.
    Coloca o file descriptor no índice 0 do array de file descriptors relativo à escrita.
    Retorna 1 em caso de insucesso ao abrir o FIFO e 0 em caso de sucesso. */
int open_client_to_server_fifo(){

    printf("[DEBUG] Opening client to server fifo...\n");
    fd[0] = open(CLIENT_TO_SERVER_FIFO, O_WRONLY);
    if(fd[0] == -1) return 1; //falhou ligação
    //printf("[DEBUG] Connection established\n");
    
    return 0;
}
int create_server_to_client_fifo(){

    int str_len = 29 + NO_DIGITS(getpid());
    // char server_to_client_fifo[str_len];
    int n = snprintf(server_to_client_fifo, str_len, SERVER_TO_CLIENT_FIFO_TEMPL"%d", getpid());
    if(n>0 && n<=str_len){
        //fprintf(stderr,"\nOpening server to client fifo...\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"%s", server_to_client_fifo);
        if(mkfifo(server_to_client_fifo, 0666)!=0){
            perror("error making fifo");
            return 1;
        }
    }
    else
        return 1;
    
    return 0;
}

int open_server_to_client_fifo(){
    fd[1] = open(server_to_client_fifo, O_RDONLY);
    if(fd[1] == -1){ 
        perror("\nFailed opening FIFO. ");
        return 1;
    }else printf("FIFO opened.\n");

    return 0;
}
 /* Envia uma request de processamento de um ficheiro ao servidor através do respetivo FIFO. */
int send_process_request(Request *request){
    
    printf("[DEBUG] Sending request\n");
    if(write(fd[0], request, sizeof(Request)+sizeof(ProcessRequestData)) <= 0) return 1;
    printf("[DEBUG] Request sent.\n");

    close(fd[0]);
    return 0; 
}
 /* Preenche os dados relativos a uma request de processamento de um ficheiro.
    Coloca a informação na struct endereçada nos argumentos. */
void create_process_request(int priority, char** execs, int total_num_transfs, char* input_path, char* output_path, Request *req){
    ProcessRequestData *req_data = (ProcessRequestData*)req->data;

    req->client_pid = getpid();
    req->priority = priority;
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
 /* Preenche os dados relativos a uma request de status.
    Coloca a informação na struct endereçada nos argumentos. */
void create_status_request(Request *req){

    req->client_pid = getpid();
    req->priority = 5;
    req->type = STATUS_REQUEST;

}
 /* Envia uma request de status ao servidor através do respetivo FIFO. */
int send_status_request(Request *req){

    //printf("[DEBUG] Sending status request\n");
    write(fd[0], req, sizeof(Request));
    printf("[DEBUG] Status request sent.\n");
    
    return 0;
}
 /* Leitura de um caractere de um dado input. */
int readc(int fd, char* c){

    return read(fd, c, 1);
}
 /* Leitura de uma linha de um dado input. */
ssize_t readln(int fd, char *buf, size_t size){
    
    int i = 0;

    while(i < (int)size && readc(fd, &buf[i]) > 0){
        i++;
        if(((char*) buf)[i-1] == '\n'){
            return i;
        }
    }

    return i;
}
 /* Leitura e escrita para o STDOUT da resposta do servidor a uma request. */
int get_response(){

    int n;
    Message message;
    while((n=read(fd[1], &message, sizeof(Message)))>0){
        //fprintf(stderr, "[DEBUG] Status response received!");
        int len = message.len;
        char data[len];
        read(fd[1], &data, sizeof(char)*len);
        write(STDOUT_FILENO,data,sizeof(char)*len);
        if(!message.wait) break;
    }
    if(n==-1){
        perror("[CLIENT->SERVER FIFO]");
        return 1;
    }

    return 0;
}
 /* Pedido de status ao servidor. 
    Devolve caso consiga*/
int get_status(Request *req){
    create_status_request(req);
    if(send_status_request(req) == 1){ perror("Unable to send request"); return 1;}
    return get_response();
}

 /* Termina o programa. 
    Unlink a FIFOs e fecha file descriptors. */
void terminate_prog(){
    close(fd[0]);
    close(fd[1]);
    unlink(SERVER_TO_CLIENT_FIFO);
}
                                    //    0      1           2               3                   4...
int main(int argc, char* argv[]){ //./sdstore proc-file -p prioridade samples/file-a outputs/file-a-output bcompress nop gcompress encrypt nop

    char req_buf[sizeof(Request)+sizeof(ProcessRequestData)];
    Request *req = (Request*)req_buf;

    int i, total_num_transfs = 0;

    if(argc > 1){
        if(strcmp(argv[1], "proc-file") == 0){
            if(strcmp(argv[2], "-p") == 0){ //com flag priority
                char* execs[argc - 6];

                for(i = 6; i < argc; i++){
                    execs[total_num_transfs] = argv[i];
                    total_num_transfs++;
                }

                // realpath falha para ficheiros não existentes
                if( open(argv[4], O_CREAT | O_RDONLY, 0666)==-1 ||
                    open(argv[5], O_CREAT | O_TRUNC, 0666)==-1)
                {
                    perror("Error opening paths");
                    return 1;
                }
                open_client_to_server_fifo(); 
                if(create_server_to_client_fifo()){
                    perror("Failed to create or open server to client fifo");
                    return 1;
                }
                create_process_request(atoi(argv[3]), execs, total_num_transfs, argv[4], argv[5], req);
                if(send_process_request(req)) perror("Unable to make request.");
                open_server_to_client_fifo();
                get_response();
                terminate_prog();
            } else{
                char* execs[argc - 4];

                for(i = 4; i < argc; i++){
                    execs[total_num_transfs] = argv[i];
                    total_num_transfs++;
                }

                if( open(argv[2], O_CREAT | O_RDONLY, 0666)==-1 ||
                    open(argv[3], O_CREAT | O_TRUNC, 0666)==-1)
                {
                    perror("Error opening paths");
                    return 1;
                }
                open_client_to_server_fifo();
                if(create_server_to_client_fifo()){
                    perror("Failed to create or open server to client fifo");
                    return 1;
                }
                create_process_request(0, execs, total_num_transfs, argv[2], argv[3], req);
                if(send_process_request(req)) perror("Unable to make request.");
                open_server_to_client_fifo();
                if(get_response()) perror("Unable to get response.");
                terminate_prog();
            }

        } else if(strcmp(argv[1], "status") == 0){
            create_server_to_client_fifo();
            create_status_request(req);
            open_client_to_server_fifo();
            send_status_request(req);
            open_server_to_client_fifo();
            get_response();
            terminate_prog();
        }
    }else{
        fprintf(stderr, "[DEBUG] wrong arg count\n");
    }
    return 0;
}