/* Simple HTTP proxy cache using LRU.
 *
 *
 * Name :     Mujing Zhou
 * Andrew ID: mujingz
*/
#include "csapp.h"
#include "cache.h"


/* cache_init -- initialize the cache for caching contents.
 * head_block is used as the block for starting iterating the 
 * cache.
 */
/* $begin cache_init */
Cache* cache_init()
{
    Cache *mycache=Malloc(sizeof(Cache));
    Block *head_block=Malloc(sizeof(Block));
    mycache->cache_head=head_block;
    mycache->cache_head->prev=NULL;
    mycache->cache_head->next=NULL;
    mycache->cache_size=0;
    mycache->block_cnt=0;
    mycache->standard_time=0;
    
    printf("cache init done\n");
    return mycache;
}
/* $end cache_init */



/* block_init -- Initialize the block used in the proxy cache.
 * uri is used as the tag for searching the cache.
 * Most recent used cache has a small access time.
 * Least recent used cache has a big access time.
 */
/* $begin block_init */
Block *block_init(char *uri,char *buf, int size)
{
    Block *new_block=Malloc(sizeof(Block));
    strcpy(new_block->uri,uri);
    memcpy(new_block->data,buf,size);
    strcpy(new_block->data,buf);
    new_block->size=size;
    new_block->prev=NULL;
    new_block->next=NULL;
    new_block->access_time=0;
    return new_block;
}
/* $end block_init */



/* in_cache -- determine whether the content has already been
 * cached. Uri is used as the tag for searching.
 * Access time of the found block is updated to 0.
 */
/* $begin in_cache */
int in_cache(Cache *proxy, char *uri_s)
{
    printf("in cache start\n");
    P(&mutex);
    readcnt++;
    if(readcnt==1)
        P(&w);
    V(&mutex);

    char tmp_uri[MAXLINE];
    Block *tmp_block=proxy->cache_head;
    
    if (proxy->block_cnt==0){
        printf("No cache: not found\n");
        
        P(&mutex);
        readcnt--;
        if(readcnt==0)
            V(&w);
        V(&mutex);
        
        return 0;
    }
    
    strcpy(tmp_uri,tmp_block->uri);
    while(tmp_block->next!=NULL){
        if (!strcmp(tmp_uri,uri_s)){
            tmp_block->access_time=0;
            printf("uri: %s is found in cache\n",uri_s);
            return 1;
        }
        tmp_block=tmp_block->next;
        strcpy(tmp_uri,tmp_block->uri);
    }
    
    P(&mutex);
    readcnt--;
    if(readcnt==0)
        V(&w);
    V(&mutex);
    
    printf("Fail to find url: %s in cache\n",uri_s);
    return 0;
}
/* $end in_cache */



/* insert -- Insert a block into the proxy cache.
 * Eviction is implemented if the size of cache exceeds 
 * MAX_CHCHE_SIZE and LRU mechanism is used.
 * Insert the block at the beginning of the cache.
 */
/* $begin insert */
void insert(Cache *proxy, char *uri, char *buf, int size){
    proxy->standard_time++;
    
    if (size>MAX_OBJECT_SIZE){
        printf("exceed object size: object size is %d\n",size);
        return;
    }
    
    if (size + proxy->cache_size > MAX_CACHE_SIZE){
        int asize=MAX_CACHE_SIZE-size;

        while(proxy->cache_size>asize){
            int last_access_time=proxy->cache_head->access_time;
            Block *tmp=proxy->cache_head;
            Block *choose=proxy->cache_head;
            
            
            
            if (tmp->next==NULL){
                free(tmp);
            }
            
            while(tmp->next!=NULL){
                P(&mutex);
            readcnt++;
            if(readcnt==1)
                P(&w);
            V(&mutex);
                if(tmp->next->access_time>last_access_time){
                    choose=tmp->next;
                    last_access_time=choose->access_time;
                    
                }
                tmp=tmp->next;
                P(&mutex);
            readcnt--;
            if(readcnt==0)
                V(&w);
            V(&mutex);
            }
            
           
            P(&w);
            tmp=choose->prev;
            tmp->next=choose->next;
            if (choose->next!=NULL)
            choose->next->prev=tmp;
            choose->prev=NULL;
            choose->next=NULL;
            proxy->cache_size-=choose->size;
            proxy->block_cnt--;
            Free(choose);
            V(&w);
        }
    }
   
 
    
    Block *tmp=proxy->cache_head;

    Block *to_be_insert=block_init(uri,buf,size);
    
    proxy->cache_size+=size;
    proxy->block_cnt++;
    
    
    if (proxy->block_cnt==0){
        printf("Empty cache, insert as the head\n");
        proxy->cache_head=to_be_insert;
        return;
    }
    
    printf("Cache not empty, insert at the beginning");
    tmp=proxy->cache_head;
    proxy->cache_head=to_be_insert;
    to_be_insert->next=tmp;
    tmp->prev=to_be_insert;
    return;
    
}
/* $end insert */



/* search -- search the proxy cache for matching uri */
/* $begin search */
Block *search(Cache *proxy, char *uri){
    
    P(&mutex);
    readcnt++;
    if(readcnt==1)
        P(&w);
    V(&mutex);
    
    Block *tmp=proxy->cache_head;
    if (tmp->next==NULL){
        printf("search: cache is empty\n");
        return NULL;
    }
    else {
        while(tmp->next!=NULL){
            if (!strcmp(uri,tmp->uri)){
                P(&mutex);
                readcnt--;
                if(readcnt==0)
                    V(&w);
                V(&mutex);
                printf("search: url: %s is found!\n",uri);
                return tmp;
            }
            tmp=tmp->next;
        }
        
        if (!strcmp(uri,tmp->uri)){
            P(&mutex);
            readcnt--;
            if(readcnt==0)
                V(&w);
            V(&mutex);
            printf("search: url: %s is found!\n",uri);
            return tmp;
        }
    }
    
    P(&mutex);
    readcnt--;
    if(readcnt==0)
        V(&w);
    V(&mutex);
    printf("search: uri not found!\n");
    return NULL;
}
/* $end search */












