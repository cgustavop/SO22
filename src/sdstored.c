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
#include "process_pipeline.h"


static_assert(sizeof(Request)+sizeof(ProcessRequestData)<=PIPE_BUF, "requests too big, non atomic writes");


static char *transf_names[] = {
    "nop\0",
    "bcompress\0",
    "bdecompress\0", 
    "gcompress\0", 
    "gdecompress\0", 
    "encrypt\0", 
    "decrypt\0"
};

char *transf_paths[TOTAL_TRANSFORMATIONS_NUMBER];

int server_pid; //para guardar o pid do servidor
char *transf_folder; //onde será guardado o path da pasta onde se encontram os executáveis das transformações
TransformationData transfs[TOTAL_TRANSFORMATIONS_NUMBER]; //lista das várias transformações
bool server_exit=false;
int request_counter=0;

PriorityQueue *requests_queue;
PriorityQueue *executing_prcs_queue;
PriorityQueue *executing_prcs_queue_swap;


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
    Request *a = *(Request**)va, *b = *(Request**)vb;

    if(a->priority>b->priority)
        return 1;
    else if(a->priority<b->priority)
        return -1;
    else
       return 0;
}

int process_prio_comp(void *va, void *vb){
    return request_prio_comp(&((Process*)va)->req, &((Process*)vb)->req);
}


void sigterm_handler(int signum){
    (void)signum;
    server_exit = true;
}

bool string_to_transformation(char *str, Transformations *out){
    for(int i=0;i<TOTAL_TRANSFORMATIONS_NUMBER;++i){
        if(strcmp(str, transf_names[i])==0){
            *out = i;
            return true;
        }
    }
    return false;
}

// <exec file name> <max. paralel instances>
int set_config(char* config_file_path){ //lê o config file linha a linha e define parâmetros para as várias transformações
    int fd = openat(AT_FDCWD, config_file_path, O_RDONLY);

    // printf("%s\n", config_file_path);
    if(fd == -1) return 1;
    char buf[64];

    while ((readln(fd, buf, 64)) > 0) {
        
        char* token = strtok(buf, " ");
        int i = 0;
        Transformations j;
        for (i = 0; i < 2 && token != NULL; i++) {
            switch (i) {
                case 0:
                    if(!string_to_transformation(token, &j))
                        continue;
                    strncpy(transfs[j].name, token, 15);
                    break;
                case 1:
                    transfs[j].max_inst = atoi(token);
                    break;
            }
            token = strtok(NULL, " "); // cleaning token
        }
        transfs[j].running_inst = 0;
    }
    close(fd);

    return 0;
}

void set_transf_paths(){
    int tr_fld_len = strlen(transf_folder);
    for(int i=0;i<TOTAL_TRANSFORMATIONS_NUMBER;++i){
        int transf_name_len = strlen(transf_names[i])+1;
        char *str = malloc(transf_name_len+tr_fld_len);
        memcpy(str, transf_folder, tr_fld_len);
        memcpy(str+tr_fld_len, transf_names[i], transf_name_len);
        transf_paths[i] = str;
    }
}

// returns pipe fd; -1==error
int open_server_to_client_fifo(Request *rq){
    char buf[64];
    int fd=-1, n = snprintf(buf, 64, SERVER_TO_CLIENT_FIFO_TEMPL"%d", rq->client_pid);
    if(n<=64 && n>0){
        fd = open(buf, O_WRONLY, 0666);
    }

    return fd;
}

// returns false on error
bool request_loop(int fifo_fd){

    while(!server_exit){
        struct pollfd pfd = {
            .events = POLLIN,
            .fd = fifo_fd,
        };
        int n=0;
        Request hdr;
        Request *p_req=NULL;
        char *read_buf = (char*)&hdr;
        ssize_t read_buf_size = sizeof(Request);

        while((n=read(fifo_fd, read_buf, read_buf_size))!=0){
            if(n==-1){
                if(errno==EAGAIN || errno==EWOULDBLOCK){
                    if(read_buf_size!=sizeof(Request) && read_buf_size!=sizeof(ProcessRequestData)){
                        poll(&pfd, 1, 20);
                    }
                    else{
                        if(pq_is_empty(executing_prcs_queue) && pq_is_empty(requests_queue)){
                            if(poll(&pfd, 1, -1)==-1){
                                perror("request loop poll error");
                            }
                        }
                        else
                            return true;                        
                    }
                }
                else{
                    perror("Client -> Server Fifo Error");
                    free(p_req);
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
                if(p_req==NULL){
                    p_req = calloc(1, sizeof(Request)+sizeof(ProcessRequestData));
                    *p_req = hdr;
                    read_buf = (char*)p_req->data;
                    read_buf_size = sizeof(ProcessRequestData);
                }
                else{
                    int fd;
                    if((fd=open_server_to_client_fifo(p_req))!=-1){
                        p_req->pipe_fd = fd;
                        p_req->num = request_counter++;
                        pq_enqueue(&p_req, requests_queue);
                        read_buf = (char*)&hdr;
                        read_buf_size = sizeof(Request);
                        p_req = NULL;
                        printf("pushed new request\n");
                        // mandar pending
                    }
                    else{
                        printf("failed to open server to client fifo\n");
                        free(p_req);
                    }
                }
            }
        }
    }
    return true;
}

Process prcs_new(Request *rq){
    Process prcs = {
        .req = rq,
        .inp_fd = open(rq->data->input, O_RDONLY, 0666),
        .out_fd = open(rq->data->output, O_WRONLY, 0666),
        .completed_num = 0,
        .is_valid = false,
    };

    if(prcs.inp_fd!=-1 && prcs.out_fd!=-1){
        prcs.is_valid = true;
        prcs.pp = pp_new(rq->data->transf_num, prcs.inp_fd, prcs.out_fd);
    }

    return prcs;
}

void prcs_add_transf(Process *prcs){
    if(!prcs->is_valid)
        return;
    for(int i=pp_get_len(prcs->pp);prcs->is_valid && i<prcs->req->data->transf_num;++i){
        Transformations tr;
        if(!string_to_transformation(prcs->req->data->transf_names[i], &tr))
            prcs->is_valid=false;
        
        else if(transfs[tr].running_inst==transfs[tr].max_inst){
            break;
        }
        
        else if(!pp_add(transf_paths[tr], (char*[]){transf_paths[tr], NULL}, prcs->pp))
            prcs->is_valid=false;
        
        else
            ++transfs[tr].running_inst;
    }
}

void prcs_free(Process prcs){
    close(prcs.inp_fd);
    close(prcs.out_fd);
    pp_free(prcs.pp);
    free(prcs.req);
}

void check_executing_prcs(){
    if(!pq_is_empty(executing_prcs_queue)){
        usleep(10*1000); // 10ms
        printf("handling execution queue\n");
        Process prcs;

        while(pq_dequeue(&prcs, executing_prcs_queue)){
            int end_num = pp_check_end_num(prcs.pp);
            for(int i=prcs.completed_num;i<end_num;++i){
                Transformations tr;
                string_to_transformation(prcs.req->data->transf_names[i], &tr);
                transfs[tr].running_inst--;
            }
            prcs.completed_num = end_num;
            if(end_num==prcs.req->data->transf_num){
                printf("prcs done\n");
                prcs_free(prcs);
            }
            else{
                if(pp_get_len(prcs.pp)<prcs.req->data->transf_num)
                    prcs_add_transf(&prcs);
                
                if(!prcs.is_valid)
                    prcs_free(prcs);
                
                pq_enqueue(&prcs, executing_prcs_queue_swap);
            }
        }
        PriorityQueue *pq = executing_prcs_queue;
        executing_prcs_queue = executing_prcs_queue_swap;
        executing_prcs_queue_swap = pq;
    }
}

void handle_new_requests(){
    Request *rq;
    while(pq_dequeue(&rq, requests_queue)){
        printf("handling new requests\n");
        if(rq->type==PROCESS_REQUEST){
            // send executing
            Process prcs = prcs_new(rq);

            prcs_add_transf(&prcs);

            if(prcs.is_valid){
                pq_enqueue(&prcs, executing_prcs_queue);
            }
            else{
                // send failure
                prcs_free(prcs);
            }
        }
    }
}

int main(int argc, char* argv[]){

    if(argc<3){
        fprintf(stderr, "Wrong arg count\n");
        return 1;
    }

    server_pid = getpid();
    transf_folder = argv[1]; //guarda o path onde estão guardadas as transformações
    printf("SETTING UP SERVER...\n");
    set_transf_paths();
    if(set_config(argv[2])){ //se retornou um valor != 0 ocorreu algum erro
        printf("Erro na leitura do ficheiro %s\n", argv[2]);
        return 1;
    }

    printf("Opening client to server fifo...\n");
    if(mkfifo(CLIENT_TO_SERVER_FIFO, 0666)!=0){
        perror("mkfifo");
    }
    int fifo_fd = open(CLIENT_TO_SERVER_FIFO, O_RDONLY | O_NONBLOCK);
    int fifo_fd_wr = open(CLIENT_TO_SERVER_FIFO, O_WRONLY | O_NONBLOCK); // keep fifo alive
    if(fifo_fd == -1){ 
        perror("Failed opening FIFO.");
        server_exit = true;
    }else printf("FIFO created.\n");

    requests_queue = pq_new(sizeof(Request*), request_prio_comp);
    executing_prcs_queue = pq_new(sizeof(Process), process_prio_comp);
    executing_prcs_queue_swap = pq_new(sizeof(Process), process_prio_comp);

    printf("\nSETUP COMPLETE...\n");

    while(!server_exit){
        printf("Searching for requests...\n");
        request_loop(fifo_fd);

        check_executing_prcs();

        handle_new_requests();
    }

    close(fifo_fd);
    close(fifo_fd_wr);
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