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

#include "request.h"
#include "priority_queue.h"


static_assert(sizeof(Request)+sizeof(ProcessRequestData)<=PIPE_BUF, "requests too big, non atomic writes");

enum transformations {nop, bcompress, bdecompress, gcompress, gdecompress, encrypt, decrypt};
#define TOTAL_TRANSFORMATIONS_NUMBER 6


typedef struct transformation {
    char name[16];
    int max_inst;
    int running_inst;
} Transformation;


int server_pid; //para guardar o pid do servidor
char *transf_folder; //onde será guardado o path da pasta onde se encontram os executáveis das transformações
Transformation transfs[TOTAL_TRANSFORMATIONS_NUMBER]; //lista das várias transformações
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

    printf("%s\n", config_file_path);
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
                Request *rq = malloc(sizeof(Request));
                *rq = hdr;
                pq_enqueue(&rq, requests_queue);
                read_buf = (char*)&hdr;
                read_buf_size = sizeof(Request);
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

    printf("Opening client to server fifo...\n");
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