/* This code is an example of producer and consumer model */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include </usr/include/asm-generic/errno-base.h>
#include "ThreadConsumerCommon.h"
#include <stdbool.h>

pthread_t producerThreadId = 0;
pthread_t consumerThreadId = 0;



static char producerReadFile[] = "testfile.mp3";
static char consumerCreatedFile[] = "outputfile.mp3";
/* START OF CRITICAL RESOURCES */
static int producerOpenedFileFd, consumerFileFd;
int32_t widx = 0, ridx = 0;
int32_t avail_size, bytes_read = 0;
int32_t bytes_to_read = 0, bytes_to_write = 0;
static char *commonBuffer = NULL;
bool producer_exited; 
/* END OF CRITICAL RESOURCES */
pthread_cond_t can_produce;
pthread_cond_t can_consume;
pthread_mutex_t ringBufferMutex;


int main(void)
{
    LOG("Starting the producer and consumer threads --");
    // create consumer thread and producer thread
    int ret = -1;
    // allocate commonBuffer
    commonBuffer = (char *)malloc(BASE_SIZE_BYTES * sizeof(char));
    if (!commonBuffer)
    {
        LOG("cannot allocate memory for commonBuffer malloc failed");
        return -ENOMEM;
    }

    // initialize the synchronization primitives
    ret = pthread_mutex_init(&ringBufferMutex, NULL);
    if (ret)
    {
        LOG("mutex initialization failed exiting");
        ret = -1;
        goto exit;
    }

    ret = pthread_cond_init(&can_produce, NULL);
    if (ret) {
        LOG("cond var thread initialization failed exiting");
        ret = -1;
        goto exit;
    }
    ret = pthread_cond_init(&can_consume, NULL);
    if (ret)
    {
        LOG("cond var thread initialization failed exiting");
        ret = -1;
        goto exit;
    }
    ret = pthread_create(&producerThreadId, NULL, producerThread, NULL);
    if (ret)
    {
        LOG("thread creation failed for producer thread - exiting");
        exit(-1);
    }

    ret = pthread_create(&consumerThreadId, NULL, consumerThread, NULL);
    if (ret)
    {
        LOG("thread creation failed for consumer thread - exiting");
        exit(-2);
    }
exit:
    if (producerThreadId)
        pthread_join(producerThreadId, NULL);
    if (consumerThreadId)
        pthread_join(consumerThreadId, NULL);
    if (commonBuffer) {
        free(commonBuffer);
    }
    return 0;
}


void *producerThread(void *args)
{
    LOG("Enter: producerThread");
    int ret;
    producerOpenedFileFd = open(producerReadFile, O_RDONLY);
    if (producerOpenedFileFd < 0) {
        LOG("open failed for %s", producerReadFile);
        goto exit;
    }
    LOG("Producer file: %s opened with fd: %d", producerReadFile, producerOpenedFileFd);
    while (1)
    {
        pthread_mutex_lock(&ringBufferMutex);
        while ((BUF_SIZE) == bytes_read) {
            LOG("waiting till consumer reads");
            pthread_cond_wait(&can_produce, &ringBufferMutex);
        }
        avail_size = BUF_SIZE - bytes_read;
        // if avail size is larger than chunk read only chunk if not read only avail_size
        bytes_to_read = (BYTES_TO_READ > avail_size) ? avail_size : BYTES_TO_READ;
        //LOG("bytes to read: %d", bytes_to_read);
        ret = read(producerOpenedFileFd, &commonBuffer[widx], bytes_to_read);
        //LOG("widx: %d", widx);
        if (ret == 0) {
            LOG("EOF reached at producer thread for file: %s", producerReadFile);
            producer_exited = true;
            pthread_cond_broadcast(&can_consume);
            pthread_mutex_unlock(&ringBufferMutex);
            goto exit;
        } else if (ret < 0) {
            LOG("read failed something's wrong with the file: %s errno: %d", producerReadFile, ret);
            pthread_mutex_unlock(&ringBufferMutex);
            producer_exited =  true;
            goto exit;
        }
        bytes_read += ret;
        widx = ((widx + ret) % (BUF_SIZE));
        //LOG("bytes read so far: %d widx: %d ret : %d", bytes_read, widx, ret);
        // write is done to commonbuffer infrom to read from buffer
        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&ringBufferMutex);
    }
exit:
    if (producerOpenedFileFd > 0)
        close(producerOpenedFileFd);
    return NULL;
}

void *consumerThread(void *args)
{
    LOG("starting consumer thread ");
    int ret;
    int consumerFileFd = open(consumerCreatedFile, O_CREAT|O_RDWR|O_TRUNC, 0644);
    if (consumerFileFd < 0)
    {
        LOG("consumer thread could not create file");
        return NULL;
    }
    LOG(" [VENU]consumer file opened: %s with fd: %d", consumerCreatedFile, consumerFileFd);

    while (1)
    {
        pthread_mutex_lock(&ringBufferMutex);
        while (bytes_read <= 0) {
            LOG("buffer empty wait till producer fills");
            if (producer_exited) {
                LOG("producer exited: %d", producer_exited);
                goto exit;
            }
            pthread_cond_wait(&can_consume, &ringBufferMutex);
        }
        bytes_to_write = (bytes_read > BYTES_TO_READ)? BYTES_TO_READ : bytes_read;
        ret = write(consumerFileFd, &commonBuffer[ridx], bytes_to_write);
        if (ret < 0){
            LOG(" bytes written is zero something's wrong");
            pthread_mutex_unlock(&ringBufferMutex);
            goto exit;
        }
        bytes_read -= ret;
        ridx = (ridx + ret) % BUF_SIZE;
        pthread_cond_signal(&can_produce);
        pthread_mutex_unlock(&ringBufferMutex);
    }
exit:
    return NULL;
}
