#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

// argv[1] First argument is number of workers (N)
// argv[2] Second argument is directory
// argv[3] Third argument is the string to find

// =================================================
// Thread Data
// =================================================
struct thread_data {
  int tid;
  char *grep_word;
};

//global scope
int num_waiting_threads = 0;
int currently_running_threads = 0;

// =================================================
// Lock Variables
// =================================================
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;

// =================================================
// Queue structure
// =================================================
typedef char *QueueElemType; // Possible memory leaker
typedef struct queuenode QueueNode;
struct queuenode{ 
    QueueElemType INFO; // Possible memory leaker
    QueueNode * LINK;
};
struct queue{
    QueueNode * front;
    QueueNode * rear;
};
typedef struct queue Queue;
void InitQueue(Queue *);
int IsEmpty(Queue *);
void ENQUEUE(Queue *, QueueElemType);
void DEQUEUE(Queue *, QueueElemType *);
void QueueOverflow(void);
void QueueUnderflow(void);
void InitQueue(Queue *Q){
    Q->front= NULL;
}
int IsEmpty(Queue *Q){
    return(Q->front == NULL);
}
void QueueOverflow(void){
    printf("Unable to get a node from the memory pool.\n");
    exit(1);
}
void QueueUnderflow(void){
    printf("Queue underflow detected.\n");
    exit(1);
}
// USE: ENQUEUE(&taskQueue, rootpath);
void ENQUEUE(Queue *Q, QueueElemType x){
    QueueNode * alpha;
    alpha = (QueueNode *) malloc(sizeof(QueueNode));
    if(alpha == NULL) QueueOverflow();
    else{
        alpha->INFO = strdup(x);
        alpha->LINK = NULL;
        if(Q->front == NULL){
            Q->front = alpha;
            Q->rear = alpha;
        }
        else{
            Q->rear->LINK = alpha;
            Q->rear = alpha;
        }
    }
}
// USE: DEQUEUE(&taskQueue, &rootpath);
void DEQUEUE(Queue *Q, QueueElemType *x){
    QueueNode * alpha;
    if (Q->front == NULL) QueueUnderflow();
    else {
        *x = Q->front->INFO;
        alpha = Q->front;
        Q->front = Q->front->LINK;
        free(alpha);
    }
}

// =================================================
// END
// =================================================

// Initializing queue
Queue taskQueue;

// =================================================
// Worker function
// =================================================
void *worker(void *arguments) { // struct thread_data *data
    pthread_mutex_lock(&mutex);
    ++currently_running_threads;
    pthread_mutex_unlock(&mutex);
    struct thread_data *args = arguments;
    int myid = args->tid;                        // Gets argument :o
    //DIR *folder;                                          // FIXME REFACTOR
    struct dirent *file;                                    // Possible memory leaker
    char actual_path [PATH_MAX + 1];
    char *absolute_path = (char*)malloc(sizeof(char));    // ALLOCATING JUST IN CASE

    // The entire mess :O
    while (1){
        QueueElemType currdir;                                  // I mean... it's kind of obvious by now, you know?
        // =========================================
        // CONSUMER
        // =========================================
        pthread_mutex_lock(&mutex);             // Not thread-safe due to... well, checking queue.
        while(IsEmpty(&taskQueue)){             // Checking if task queue is full
            ++num_waiting_threads;
            while (num_waiting_threads == currently_running_threads){
                --currently_running_threads;
                --num_waiting_threads;
                //printf("Q: Thread %d exiting. Number of waiting threads: %d\n", myid, num_waiting_threads);
                pthread_cond_signal(&fill);
                pthread_mutex_unlock(&mutex);
                return 0;
            }
            pthread_cond_wait(&fill, &mutex);
            --num_waiting_threads;
        }
        DEQUEUE(&taskQueue, &currdir);          // Dequeuing queue
        
        // BREAKING CHANGE: ORIGINAL SPOT IS INDICATED BELOW
        absolute_path = realpath(currdir, actual_path);
        char *dirtemp = (char*)malloc(sizeof(char) * 2000);
        snprintf(dirtemp, 2000, "%s", absolute_path);
        printf("[%d] DIR %s\n", myid, dirtemp);
        free(dirtemp);
        // END BREAKING CHANGE

        // =========================================
        // PRODUCER... possibly
        // =========================================
        DIR *folder = opendir(currdir);            // Open folder
        
        // ORIGINAL SPOT OF THE FOLLOWING CHANGE
        free(currdir);

        //pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);           // Is now safe.
        
        while((file = readdir(folder))){                                        // It's... kind of expected na that we're pretty much... ya know.
            if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, "..")){     // In case of . and ..
                continue;
            }

            // Memory ownership
            char *temp = (char*)malloc(sizeof(char) * 2000);
            snprintf(temp, 2000, "%s/%s", absolute_path,file->d_name);

            // If d_type == 4 (which means directory)
            if (file->d_type == 4){                         
                pthread_mutex_lock(&mutex);
                
                //pthread_cond_wait(&empty, &mutex);          
                printf("[%d] ENQUEUE %s\n", myid, temp);
                
                ENQUEUE(&taskQueue, temp);
                pthread_cond_signal(&fill);                                     // Calling signal if it is empty.
                pthread_mutex_unlock(&mutex);                                   // Is now safe.
            }

            // If d_type == 8 (which means file)
            if (file->d_type == 8){                          
                char *greptemp = (char*)malloc(sizeof(char) * 2000);
                
                snprintf(greptemp, 2000 , "grep -c -q \"%s\" \"%s/%s\" > /dev/null", args->grep_word, absolute_path,file->d_name);
                
                if (system(greptemp) == 0){
                    printf("[%d] PRESENT %s\n", myid, temp);  
                }
                else{
                    printf("[%d] ABSENT %s\n", myid, temp);
                }
                free(greptemp);
            }
            free(temp);
            
        }
        
        closedir(folder);
    
    }
}


int main(int argc, char *argv[]){
    // If the following arguments are not equal to what was expected
    if (argc != 4)
    {
      fprintf(stderr, "Wrong number of arguments\n");
      exit(EXIT_FAILURE);
    }
    
    // Initialization of threads
    int numThreads = atoi(argv[1]);
    pthread_t thread[numThreads];
    struct thread_data data[numThreads];
    
    InitQueue(&taskQueue);
    char tempo [PATH_MAX + 1];
    QueueElemType rootpath = realpath(argv[2], tempo);
    ENQUEUE(&taskQueue, rootpath);
    // // Debugger   
    // DEQUEUE(&taskQueue, &rootpath);
    // printf("%s\n", rootpath);
    for (int i = 0; i < numThreads; i++) {
        data[i].tid = i;
        data[i].grep_word = argv[3];
        //free(arg);
        //pthread_create(&thread[i], NULL, worker, arg);
        pthread_create(&thread[i], NULL, worker, &data[i]);        
    }
    for (int i = 0; i < numThreads; i++) {
        pthread_join(thread[i], NULL);
    }

    return 0;
}