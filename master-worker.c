#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int item_to_produce = 0, curr_buf_size = 0;
int total_items, max_buf_size, num_workers, num_masters;

int *buffer;

// Initialize synchronization variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

void print_produced(int num, int master) {
    printf("Produced %d by master %d\n", num, master);
}

void print_consumed(int num, int worker) {
    printf("Consumed %d by worker %d\n", num, worker);
}

// Produce items and place them in the buffer
void *generate_requests_loop(void *data) {
    int thread_id = *((int *)data);

    while (1) {
        pthread_mutex_lock(&mutex);

        if (item_to_produce >= total_items) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        while (curr_buf_size == max_buf_size) {
            pthread_cond_wait(&full, &mutex);
        }

        buffer[curr_buf_size++] = item_to_produce;
        print_produced(item_to_produce, thread_id);
        item_to_produce++;

        pthread_cond_signal(&empty);

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

// Worker consumer thread function
void *consume_requests_loop(void *data) {
    int thread_id = *((int *)data);
    int item;

    while (1) {
        pthread_mutex_lock(&mutex);

        // Exit condition: If all items have been consumed
        if (item_to_produce >= total_items && curr_buf_size == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Wait if the buffer is empty
        while (curr_buf_size == 0) {
            if (item_to_produce >= total_items) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            pthread_cond_wait(&empty, &mutex);
        }

        // Consume item from the buffer
        item = buffer[--curr_buf_size];
        print_consumed(item, thread_id);

        // Signal that the buffer is not full
        pthread_cond_signal(&full);

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

//write function to be run by worker threads
//ensure that the workers call the function print_consumed when they consume an item
int main(int argc, char *argv[]) {
    int *master_thread_id;
    int *worker_thread_id;
    pthread_t *master_thread;
    pthread_t *worker_thread;

    if (argc < 5) {
        printf("./master-worker #total_items #max_buf_size #num_workers #masters e.g. ./exe 10000 1000 4 3\n");
        exit(1);
    }

    total_items = atoi(argv[1]);
    max_buf_size = atoi(argv[2]);
    num_workers = atoi(argv[3]);
    num_masters = atoi(argv[4]);

    buffer = (int *)malloc(sizeof(int) * max_buf_size);

    //create master producer threads
    master_thread_id = (int *)malloc(sizeof(int) * num_masters);
    master_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_masters);
    worker_thread_id = (int *)malloc(sizeof(int) * num_workers);
    worker_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_workers);

    // Create master threads
    for (int i = 0; i < num_masters; i++) {
        master_thread_id[i] = i;
        pthread_create(&master_thread[i], NULL, generate_requests_loop, &master_thread_id[i]);
    }

    //create worker consumer threads
    for (int i = 0; i < num_workers; i++) {
        worker_thread_id[i] = i;
        pthread_create(&worker_thread[i], NULL, consume_requests_loop, &worker_thread_id[i]);
    }

    // Wait for all master threads to complete
    for (int i = 0; i < num_masters; i++) {
        pthread_join(master_thread[i], NULL);
        printf("master %d joined\n", i);
    }

    // Wait for all worker threads to complete
    for (int i = 0; i < num_workers; i++) {
        pthread_join(worker_thread[i], NULL);
        printf("worker %d joined\n", i);
    }

    /*----Deallocating Buffers---------------------*/
    free(buffer);
    free(master_thread_id);
    free(master_thread);
    free(worker_thread_id);
    free(worker_thread);

    return 0;
}
