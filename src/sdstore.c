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
    Retorna false em caso de insucesso ao abrir o FIFO e true em caso de sucesso. */
bool open_client_to_server_fifo(){

    //printf("[DEBUG] Opening client to server fifo...\n");
    fd[0] = open(CLIENT_TO_SERVER_FIFO, O_WRONLY);
    if(fd[0] == -1) return false; //falhou ligação
    //printf("[DEBUG] Connection established\n");
    
    return true;
}
bool create_server_to_client_fifo(){

    int str_len = 29 + NO_DIGITS(getpid());
    int n = snprintf(server_to_client_fifo, str_len, SERVER_TO_CLIENT_FIFO_TEMPL"%d", getpid());
    if(n>0 && n<=str_len){
        if(mkfifo(server_to_client_fifo, 0666)!=0){
            perror("error making fifo");
            return false;
        }
    }
    else
        return false;
    
    return true;
}

bool open_server_to_client_fifo(){
    fd[1] = open(server_to_client_fifo, O_RDONLY);
    if(fd[1] == -1){ 
        perror("\nFailed opening FIFO. ");
        return false;
    }

    return true;
}
 /* Envia uma request de processamento de um ficheiro ao servidor através do respetivo FIFO. */
bool send_process_request(Request *request){
    
    //printf("[DEBUG] Sending request\n");
    if(write(fd[0], request, sizeof(Request)+sizeof(ProcessRequestData)) <= 0) return false;
    //printf("[DEBUG] Request sent.\n");

    close(fd[0]);
    return true; 
}
 /* Preenche os dados relativos a uma request de processamento de um ficheiro.
    Coloca a informação na struct endereçada nos argumentos. */
bool create_process_request(int priority, char** execs, int total_num_transfs, char* input_path, char* output_path, Request *req){
    ProcessRequestData *req_data = (ProcessRequestData*)req->data;

    req->client_pid = getpid();
    req->priority = priority;
    req->type = PROCESS_REQUEST;

    for(int i = 0; i < total_num_transfs; i++){
        Transformations tr;
        if(string_to_transformation(execs[i], &tr))
            req_data->transfs[i] = tr;
        else
            return false;
    }
    char *inp = realpath(input_path, NULL);
    char *outp = realpath(output_path, NULL);
    strncpy(req_data->input, inp, strlen(inp)+1);
    strncpy(req_data->output, outp, strlen(outp)+1);
    free(inp);
    free(outp);

    req_data->transf_num = total_num_transfs;
    
    return true;
}
 /* Preenche os dados relativos a uma request de status.
    Coloca a informação na struct endereçada nos argumentos. */
void create_status_request(Request *req){

    req->client_pid = getpid();
    req->priority = 5;
    req->type = STATUS_REQUEST;

}
 /* Envia uma request de status ao servidor através do respetivo FIFO. */
bool send_status_request(Request *req){

    //printf("[DEBUG] Sending status request\n");
    if(write(fd[0], req, sizeof(Request))==-1) return false;
    //printf("[DEBUG] Status request sent.\n");
    
    return true;
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
bool get_response(){

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
        return false;
    }

    return true;
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

    int arg_iter=1;

    if(argc > 1){
        if(strcmp(argv[arg_iter], "proc-file")==0){
            
            arg_iter++;
            
            int priority=0;

            if(strcmp(argv[arg_iter], "-p")==0){
                arg_iter++;
                priority = atoi(argv[arg_iter++]);
                priority =  priority>5 ? 
                                5 : priority<0 ? 
                                    0 : priority;
            }

            char *inp_path = argv[arg_iter++];
            char *out_path = argv[arg_iter++];

            int fd1=-1, fd2=-1;
            // realpath falha para ficheiros não existentes
            if( (fd1=open(inp_path, O_CREAT | O_RDONLY, 0666))==-1 ||
                (fd2=open(out_path, O_CREAT | O_TRUNC, 0666))==-1)
            {
                perror("Error opening paths");
                return 1;
            }
            close(fd1);
            close(fd2);

            if( !create_process_request(priority, argv+arg_iter, argc-arg_iter, inp_path, out_path, req) ||
                !create_server_to_client_fifo() || 
                !open_client_to_server_fifo() || 
                !send_process_request(req) ||
                !open_server_to_client_fifo() ||
                !get_response())
            {
                fprintf(stderr, "Failed to make request\n");
            }
            terminate_prog();
        }
        else if(strcmp(argv[1], "status")==0){
            create_status_request(req);
            if( !create_server_to_client_fifo() ||
                !open_client_to_server_fifo() || 
                !send_status_request(req) ||
                !open_server_to_client_fifo() ||
                !get_response())
            {
                fprintf(stderr, "Failed to make request\n");
            }
            terminate_prog();
        }
    }
    else{
        fprintf(stderr, "Wrong arg count\n");
    }
    return 0;
}