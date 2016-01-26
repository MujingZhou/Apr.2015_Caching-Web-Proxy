/* Simple HTTP proxy that caches web objects.
 * 
 *
 * Name :     Mujing Zhou
 * Andrew ID: mujingz
 *
 *
 *
 * Client: Web browser
 * Proxy : This program run on shark machine
 * Server: Web server you visit from browser
 *
 *
 * Step 1:
 * Simple proxy based on TINY. Read and parse requests from web browser
 * and reorganize the headers that are sended to server. Read and directly
 * forward contents from server to web browser.
 *
 * Step 2:
 * Proxy based on Step 1 and concurency is added. Multiple concurrent connections
 * are dealt with.
 *
 * Step 3:
 * Proxy based on Step 2 and cache is added to cache web contents. LRU caching
 * mechanism is used for eviction blocks from cache. Race conditions considered.
 *
 */
#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* Default port number */
#define PORT_DEFAULT "80"

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *conn = "Connection: close\r\n";
static const char *proxy_conn = "Proxy-Connection: close\r\n";

/* Helper functions */

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void doit(int fd);
void doit_cached (Cache *proxy, char *uri, int fd_client);
void hdr_concat(rio_t *rio_client,char *hdr_server,char *hostname, char *port);
void parse_uri(char *uri, char *pathname, char *hostname, char *port);
void *thread(void *vargp);

/* Global variables defined */
Cache *mycache;
sem_t mut_fd;



/* main -- main funtion to establish connection between 
 * proxy and client. 
 * SIGPIPE is ignored so that proxy will not terminate 
 * due to broken pipe error.
 */
/* $begin main */
int main(int argc,char **argv)
{
    readcnt=0;
    struct sockaddr_in clientaddr;
    int listenfd,*connfd;
    socklen_t clientlen;
    char* port;
    pthread_t tid;
    
    /* Semaphores initialized here.
     * mut_fd to protect connfd
     * mutex and w are used in cache
     */
    sem_init(&mutex,0,1);
    sem_init(&mut_fd,0,1);
    sem_init(&w,0,1);
    
    mycache=cache_init();
    Signal(SIGPIPE, SIG_IGN);
    
    if (argc!=2){
        fprintf(stderr,"usage: %s <port>\n",argv[0]);
        exit(1);
    }
    
    port=argv[1];
    
    listenfd=open_listenfd(port);
    while(1){
        connfd=Malloc(sizeof(int));
        clientlen=sizeof(clientaddr);
        P(&mut_fd);
        *connfd=Accept(listenfd,(SA *)&clientaddr,&clientlen);
        Pthread_create(&tid,NULL,thread,connfd);
    }
    return 0;
}
/* $end main */


/* doit_cached -- if content is cached, directly forward back
 * to client.
 */
/* $begin doit_cached */
void doit_cached (Cache *proxy, char *uri, int fd_client) {
    Block *found_block =search(proxy, uri);
    if (rio_writen(fd_client, found_block->data, found_block->size) < 0) {
        printf("Error writing from cache to client\n");
    }
}
/* $end doit_cached */



/* doit -- main function for implementing fetching and 
 * forwarding. If the content is already cached, directly forward
 * the cached content to client. If the content is not cached, 
 * forward the content back as well as inserting the content into
 * the cache.
 * memcpy is used to tranfer content including images.
 */
/* $begin doit */
void doit(int fd_client)
{
    rio_t rio_client;
    rio_t rio_server;
    
    int fd_server=0;
    int is_cached=0;
    
    char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
    char pathname[MAXLINE],hostname[MAXLINE],hdr_server[MAXLINE];
    char *port=(char*)malloc(sizeof(port));
    
    strcpy(port,PORT_DEFAULT);
    
    rio_readinitb(&rio_client,fd_client);
    rio_readlineb(&rio_client,buf,MAXLINE);
    
    sscanf(buf,"%s %s %s",method,uri,version);
    
    if (strcasecmp(method, "GET")) {
        clienterror(fd_client, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }
    
    is_cached = in_cache(mycache, uri);
    if (is_cached) {
        printf("Cache hit\n");
        doit_cached(mycache, uri, fd_client);
    }
    
    else {
    printf("Cache miss\n");
    parse_uri(uri,pathname,hostname,port);
    
    
    fd_server=Open_clientfd(hostname,port);
    hdr_concat(&rio_client,hdr_server,hostname,pathname);
    // printf("%s",hdr_server);
    rio_writen(fd_server, hdr_server, strlen(hdr_server));
    rio_readinitb(&rio_server,fd_server);
    
    char buf_server[MAXLINE],file_buf[MAX_OBJECT_SIZE];
    int n=0;
    int size=0;
    memset(buf_server, 0, MAXLINE);
        
    while((n=rio_readlineb(&rio_server,buf_server,MAXLINE))!=0){
        if (size+n <= MAX_OBJECT_SIZE) {
            memcpy(file_buf+size, buf_server, n);
            size += n;
        }
        rio_writen(fd_client, buf_server, n);
        memset(buf_server, 0, n);
    }
        if (size <= MAX_OBJECT_SIZE) {
            insert(mycache, uri, file_buf, size);
        }
    close(fd_server);
    free(port);
    }
}
/* $end doit */



/* thread -- function for implementing concurrency */
/* $begin thread */
void *thread(void *vargp) {
    int connfd = *((int *)vargp);
    V(&mut_fd);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    printf("connfd: %d is closed\n",connfd);
    close(connfd);
    return NULL;
}
/* $end thread */



/* hdr_concat -- reorganize the header that would be sent
 * to the server. With the exception of the Host header, 
 * the proxy ignore the values of the request "User-Agent:",
 * "Accept:", "Accept-Encoding:", "Connection:" and 
 * "Proxy-Connection:".
 */
/* $begin hdr_concat */
void hdr_concat(rio_t *rio_client,char *hdr_server,char *hostname, char *pathname)
    {
        char add_info[MAXLINE];
        char buf[MAXLINE];
        char *ptr;
        int n=0;
        int off=0;
        if (rio_readlineb(rio_client,buf,MAXLINE)!=0){
            
            ptr=buf;
            if (!strncmp(buf,"Host:",5)){
                ptr+=5;
                *ptr='\0';
                strstr(hostname,ptr);
            }
        }
        
        memset(buf,0,MAXLINE);
        memset(add_info,0,MAXLINE);
        while((n=rio_readlineb(rio_client,buf,MAXLINE))>0){
            if (!strcmp(buf,"\r\n"))
                break;
            ptr=buf;
            if (!strncmp(buf,"User-Agent:",11)||
                !strncmp(buf,"Accept:",7)||
                !strncmp(buf,"Accept-Encoding:",16)||
                !strncmp(buf,"Connection:",11)||
                !strncmp(buf,"Proxy-Connection:",17)){
                continue;
            }
            else{
                memcpy(add_info+off,buf,n);
                memset(buf,0,MAXLINE);
                off+=n;
            }
            
        }

        sprintf(hdr_server,"GET %s HTTP/1.0\r\n",pathname);
        sprintf(hdr_server,"%sHost: %s\r\n",hdr_server,hostname);
        strcat(hdr_server,user_agent_hdr);
        strcat(hdr_server,accept_hdr);
        strcat(hdr_server,accept_encoding_hdr);
        strcat(hdr_server,conn);
        strcat(hdr_server,proxy_conn);
        strcat(hdr_server,"\r\n");
        strcat(hdr_server,add_info);
    }
/* $end hdr_concat */



/* parse_uri -- parse uri into hostname, pathname and 
 * possibly port number.
 */
/* $begin parse_uri */
void parse_uri(char *uri, char *pathname, char *hostname,char *port)
{
    char buf[MAXLINE];
    char *ptr;
    
    
    strcpy(buf,uri);
    ptr=buf;
    if (strstr(uri,"http://")==uri){
        ptr+=strlen("http://");
    }
    
    else {
        printf("not start with http://\n");
        exit(1);
    }
    
    while (*ptr != ':'&& *ptr != '/'){
        *hostname++ = *ptr++;
    }
    *hostname = '\0';
    
    if (*ptr=='/'){
        while (*ptr!='\0') {
            *pathname ++ = *ptr ++;
        }
    }
    
    if (*ptr==':'){
        ptr++;
        while (*ptr!='/'){
            *port++=*ptr++;
        }
        
        *port='\0';
        
        if (*ptr=='/'){
            while (*ptr!='\0') {
                *pathname ++ = *ptr ++;
            }
        }
    }
}
/* $end parse_uri */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];
    
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    
    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */


















