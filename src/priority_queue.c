#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct priorityQueue{
    char *arr;
    void *swap_buf;
    size_t data_size;
    size_t arr_size;
    size_t arr_len;
    int(*comparator)(void*, void*);
}PriorityQueue;


PriorityQueue *pq_new(size_t data_size, int(comparator)(void*, void*)){
    PriorityQueue *pq = malloc(sizeof(PriorityQueue));
    pq->arr = malloc(data_size*4);
    pq->swap_buf = malloc(data_size);
    pq->data_size = data_size;
    pq->arr_size = 4;
    pq->arr_len = 0;
    pq->comparator = comparator;
    return pq;
}

void pq_free(PriorityQueue *pq){
    free(pq->arr);
    free(pq->swap_buf);
    free(pq);
}

void pq_enqueue(void *data, PriorityQueue *pq){
    if(pq->arr_len==pq->arr_size){
        pq->arr_size *= 2; 
        pq->arr = realloc(pq->arr, pq->data_size*pq->arr_size);
    }
    memcpy(pq->arr+pq->arr_len*pq->data_size, data, pq->data_size);

    ++pq->arr_len;

    for(size_t ind = pq->arr_len-1;
            ind!=0 && pq->comparator(data, pq->arr+(ind-1)/2*pq->data_size)>0;
            ind = (ind-1)/2)
    {
        memcpy(pq->arr+ind*pq->data_size, pq->arr+(ind-1)/2*pq->data_size, pq->data_size);
        memcpy(pq->arr+(ind-1)/2*pq->data_size, data, pq->data_size);
    }
}

bool pq_dequeue(void *data_out, PriorityQueue *pq){

    if(pq->arr_len==0)
        return false;
    
    memcpy(data_out, pq->arr, pq->data_size);

    void *last = pq->swap_buf;

    --pq->arr_len;

    memcpy(last, pq->arr+pq->arr_len*pq->data_size, pq->data_size);

    for(size_t ind=0;ind<pq->arr_len;){

        size_t next_ind, left = ind*2+1, right = ind*2+2;

        if(right<pq->arr_len)
            next_ind = pq->comparator(pq->arr+left*pq->data_size,pq->arr+right*pq->data_size)>=0 ? 
                left: 
                right;
        else if(left<pq->arr_len)
            next_ind = left;
        else
            next_ind = 0;

        if(next_ind != 0 && pq->comparator(pq->arr+next_ind*pq->data_size, last)>0){
            memcpy(pq->arr+ind*pq->data_size, pq->arr+next_ind*pq->data_size, pq->data_size);
        }
        else{
            memcpy(pq->arr+ind*pq->data_size, last, pq->data_size);
            break;
        }

        ind = next_ind;
    }

    if(pq->arr_len<pq->arr_size/3){
        pq->arr_size = pq->arr_size/2;
        pq->arr = realloc(pq->arr, pq->data_size * pq->arr_size);
    }

    return true;
}

bool pq_peek(void* data_out, PriorityQueue *pq){
    if(pq->arr_len>0){
        memcpy(data_out, pq->arr, pq->data_size);
        return true;
    }
    else
        return false;
}

bool pq_is_empty(PriorityQueue *pq){
    if(pq->arr_len>0)
        return false;
    else
        return true;
}

void *pq_get_arr(size_t *arr_len_out, PriorityQueue *pq){
    *arr_len_out = pq->arr_len;
    return pq->arr;
}

void *pq_iter_start(PriorityQueue *pq){
    return pq->arr;
}

void *pq_iter_end(PriorityQueue *pq){
    return pq->arr+(pq->arr_len*pq->data_size);
}

#define PQ_FOREACH(var_name, type, prio_queue) \
    for(type *var_name=pq_iter_start(prio_queue);var_name<(type*)pq_iter_end(prio_queue);++var_name)

int int_comparator(void *a, void *b){
    int *na = a, *nb = b;

    if(*na>*nb)
        return 1;
    else if(*na<*nb)
        return -1;
    else
        return 0;
}

// int main(){
//     PriorityQueue *pq = pq_new(sizeof(int),int_comparator);

//     int len = 10;

//     for(int i=0;i<len;++i){
//         pq_enqueue(&i, pq);
//     }

//     printf("arr_size:%ld\narr_len:%ld\n", pq->arr_size, pq->arr_len);

//     // printf("arr: ");
//     // for(int i=0;i<10;++i){
//     //     printf("%d ", *(pq->arr+sizeof(int)*i));
//     // }
//     // printf("\n");

//     PQ_FOREACH(iter, int, pq){
//         printf("%d ", *iter);
//     }
//     printf("\n");

//     for(int i=len-1, buf;i>=0;--i){
//         pq_dequeue(&buf, pq);
//         assert(buf==i);
//         // printf("i:%d, buf:%d\n", i, buf);
//         // printf("arr: ");
//         // for(int i=0;i<pq->arr_len;++i){
//         //     printf("%d ", *(pq->arr+sizeof(int)*i));
//         // }
//         // printf("\n\n");
//     }

//     printf("arr_size:%ld\narr_len:%ld\n", pq->arr_size, pq->arr_len);

//     assert(pq_dequeue(NULL, pq)==false);

//     pq_free(pq);

//     return 0;
// }