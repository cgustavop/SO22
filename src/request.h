#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdbool.h>
#include "process_pipeline.h"
#include <math.h>

#define NO_DIGITS(x) (x==0?1:(int)(log10(x)+1))
#define TOTAL_TRANSFORMATIONS 7

#define PATH_SIZE 1024
#define CLIENT_TO_SERVER_FIFO "/tmp/client_to_server_fifo" //path para o fifo que permite a comunicação entre o cliente e o servidor
#define SERVER_TO_CLIENT_FIFO_TEMPL "/tmp/server_to_client_fifo_"


typedef enum MessageType{
    MSG_STRING, //envio de notificações ("pending" e "processing")
    MSG_SERVER_STATUS //resposta a um pedido de status
}MessageType;

typedef enum Status{
    FAILURE,
    PENDING,
    PROCESSING,
    CONCLUDED
}Status;

typedef struct Message{
    bool wait; // wait true, o cliente espera por mais mensagens (vice-versa)
    int len;
    char data[];
}Message;

typedef enum RequestType{
    STATUS_REQUEST,
    PROCESS_REQUEST
}RequestType;

typedef struct processRequestData {
    int transf_num;
    int input_len;
    int output_len;
    char transf_names[16][16];
    char input[PATH_SIZE];
    char output[PATH_SIZE];
} ProcessRequestData;

typedef struct request {
    RequestType type;
    int client_pid;
    int priority;
    ProcessRequestData data[];
}Request;

typedef struct process {
    Request *req;
    ProcessPipeline *pp;
    Status status;
    int inp_fd;
    int out_fd;
    int prcs_num;
    int pipe_fd;
    int completed_num;
    bool updateClientStatus;
    bool is_valid;
}Process;

typedef enum transformations {
    nop=0,
    bcompress=1,
    bdecompress=2, 
    gcompress=3, 
    gdecompress=4, 
    encrypt=5, 
    decrypt=6
}Transformations;

typedef struct transformationData {
    char name[16];
    int max_inst;
    int running_inst;
} TransformationData;

#endif