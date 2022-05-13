#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <errno.h>
#include <assert.h>
#include <poll.h>
#include <math.h>

#include "request.h"
#include "priority_queue.h"

static_assert(sizeof(Request)+sizeof(ProcessRequestData)<=PIPE_BUF, "requests too big, non atomic writes");

int server_pid; //para guardar o pid do servidor
char *transf_folder; //onde será guardado o path da pasta onde se encontram os executáveis das transformações
Transformation transfs[TOTAL_TRANSFORMATIONS]; //lista das várias transformações
bool server_exit=false;

PriorityQueue *requests_queue;


int readc(int fd, char* c){

    return read(fd, c, 1);
}

ssize_t readln(int fd, char *buf, size_t size){
    
    size_t i = 0;

    while(i < size && readc(fd, &buf[i]) > 0){
        i++;
        if(((char*) buf)[i-1] == '\n'){
            return i;
        }
    }

    return i;
}

int request_prio_comp(void *va, void *vb){
    Request *a = va, *b = vb;

    if(a->priority>b->priority)
        return 1;
    else if(a->priority<b->priority)
        return -1;
    else
       return 0;
}


void sigterm_handler(int signum){
    (void)signum;
    server_exit = true;
}


// <exec file name> <max. paralel instances>
int set_config(char* config_file_path){ //lê o config file linha a linha e define parâmetros para as várias transformações
    int fd = openat(AT_FDCWD, config_file_path, O_RDONLY);

    //printf("%s\n", config_file_path);
    if(fd == -1) return 1;
    char buf[64];

    while ((readln(fd, buf, 64)) > 0) {
        
        char* token = strtok(buf, " ");
        int i, j = 0;
        for (i = 0; i < 2 && token != NULL; i++) {
            switch (i) {
                case 0:
                    strncpy(transfs[j].name, token, 15);
                    break;
                case 1:
                    transfs[j].max_inst = atoi(token);
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

int open_server_to_client_fifo(int client_pid){
    int fd;
    int str_len = 23 + NO_DIGITS(client_pid);
    char SERVER_TO_CLIENT_FIFO[str_len];
    snprintf(SERVER_TO_CLIENT_FIFO, str_len, "server_to_client_fifo_%d", client_pid);
    fprintf(stderr,"[DEBUG] FIFO path: %s\n", SERVER_TO_CLIENT_FIFO);
    
    while((fd = open(SERVER_TO_CLIENT_FIFO, O_WRONLY)) == -1){
        sleep(1);
        fprintf(stderr,"[DEBUG] Trying to connect to server\n");
    }
    
    return 0;
}
int get_status(char *response){
    int res_len = 6;
    char teste[6] = "teste";
    memcpy(response, teste, res_len);
    return res_len;
}

//int fstat(int fd, struct stat *buf);
void get_IO_bytes_info(char *response, int input_fd, int output_fd){
    struct stat *input_stat = malloc(sizeof(struct stat));
    struct stat *output_stat = malloc(sizeof(struct stat));

    fstat(input_fd, input_stat);
    fstat(output_fd, output_stat);

    int input_size = input_stat->st_size;
    int output_size = output_stat->st_size;

    int res_len = 32 + NO_DIGITS(input_size) + NO_DIGITS(output_size);
    response = malloc(sizeof(char)*res_len);

    snprintf(response, res_len, "(bytes-input: %d, bytes-output: %d)", input_size, output_size);
}

void send_response(int client_pid, char response[], int response_len){
    
    char res_buf[sizeof(Message)+(sizeof(char)*response_len)];
    Message *message = (Message*)res_buf;

    message->len = response_len;
    //message->data[25] = "status request recebido";
    memcpy(message->data, response, (sizeof(char)*response_len));
    int client_fifo_fd = open_server_to_client_fifo(client_pid); //TODO cliente cria o fifo e não o servidor

    write(client_fifo_fd, message, sizeof(res_buf));
}

//void send_proc_status(int input_fd,int output_fd,int client_pid, Status status){
void send_proc_status(Request req){
    int res_len = 0;
    ProcessRequestData *data = req.data;
    switch(data->status){
        case PENDING:
            res_len = 8;
            send_response(req.client_pid, "pending", res_len);
            break;
        
        case PROCESSING:
            res_len = 11;
            send_response(req.client_pid, "processing", res_len);
            break;
        case CONCLUDED:
            res_len = 11;
            char *response = strndup("concluded ", res_len);
            char *bytes = NULL;
            get_IO_bytes_info(bytes, data->input_fd, data->output_fd);
            send_response(req.client_pid, response, res_len);
            free(response);

            break;
    }
}


bool request_loop(int fifo_fd){

    while(pq_is_empty(requests_queue) && !server_exit){
        struct pollfd pfd = {
            .events = POLLIN,
            .fd = fifo_fd,
        };
        poll(&pfd, 1, -1);
        int n=0;
        Request hdr;
        char *read_buf = (char*)&hdr;
        ssize_t read_buf_size = sizeof(Request);
        bool reading_process_data = false;
        while((n=read(fifo_fd, read_buf, read_buf_size))!=0){
            if(n==-1){
                if(errno==EAGAIN || errno==EWOULDBLOCK){
                    continue;
                }
                else{
                    perror("Client -> Server Fifo");
                    return false;
                }
            }
            else if(n!=read_buf_size){
                read_buf_size = read_buf_size-n;
                read_buf += n;
            }
            else if(hdr.type==STATUS_REQUEST){
                fprintf(stderr,"[DEBUG] Recebi uma status request");
                char *response = NULL;
                int res_len = get_status(response);
                send_response(hdr.client_pid, response, res_len);
                free(response);
            }
            else if(hdr.type==PROCESS_REQUEST){
                Request *p_rq;
                if(!reading_process_data){
                    p_rq = calloc(1, sizeof(Request)+sizeof(ProcessRequestData));
                    *p_rq = hdr;
                    read_buf = (char*)p_rq->data;
                    read_buf_size = sizeof(ProcessRequestData);
                    reading_process_data = true;
                }
                else{
                    pq_enqueue(&p_rq, requests_queue);
                    read_buf = (char*)&hdr;
                    read_buf_size = sizeof(Request);
                    reading_process_data = false;
                    fprintf(stderr, "received\n");
                }
            }
        }
    }
    return true;
}


int main(int argc, char* argv[]){

    if(argc<3){
        fprintf(stderr, "Wrong arg count\n");
        return 1;
    }

    server_pid = getpid();
    transf_folder = argv[1]; //guarda o path onde estão guardadas as transformações
    printf("SETTING UP SERVER...\n");
    if(set_config(argv[2])){ //se retornou um valor != 0 ocorreu algum erro
        printf("Erro na leitura do ficheiro %s\n", argv[2]);
        return 1;
    }

    //printf("Opening client to server fifo...\n");
    if(mkfifo(CLIENT_TO_SERVER_FIFO, 0666)!=0){
        perror("mkfifo");
    }
    int fd = open(CLIENT_TO_SERVER_FIFO, O_RDONLY | O_NONBLOCK);
    if(fd == -1){ 
        perror("Failed opening FIFO.");
        server_exit = true;
    }else printf("FIFO created.\n");

    requests_queue = pq_new(sizeof(Request*), request_prio_comp);

    printf("\nSETUP COMPLETE...\n");

    while(!server_exit){
        printf("Searching for requests...\n");
        request_loop(fd);
        // teste
        Request *rq;
        while(pq_dequeue(&rq, requests_queue)){
            printf("client pid:%d\n",rq->client_pid);
            free(rq);
        }
    }

    return 0;
}