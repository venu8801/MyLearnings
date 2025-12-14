#define LOG(str, ...) printf(str "\n", ##__VA_ARGS__)

#define BASE_SIZE_BYTES 1024
#define BYTES_TO_READ 24
#define BUF_SIZE (BASE_SIZE_BYTES * sizeof(char))
void * producerThread(void *);
void * consumerThread(void *);