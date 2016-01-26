#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

sem_t mutex,w;
int readcnt;
typedef struct cache_block{
    struct cache_block *prev;
    struct cache_block *next;
    char data[MAX_OBJECT_SIZE];
    char uri[MAXLINE];
    int size;
    int access_time;
} Block;

typedef struct cache{
    Block *cache_head;
    int cache_size;
    int block_cnt;
    sem_t mutex;
    int standard_time;
} Cache;

Cache* cache_init();
Block *block_init(char *uri,char *buf, int size);
int in_cache(Cache *proxy, char *uri_s);
void insert(Cache *proxy, char *uri, char *buf, int size);
Block *search(Cache *proxy, char *uri);