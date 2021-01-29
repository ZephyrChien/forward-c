#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SRCADDR "127.0.0.1"
#define DSTADDR "127.0.0.1"
#define SRCPORT 10000
#define DSTPORT 20000

typedef struct {
    int srcfd;
    int dstfd;
    // char *info;
} proxy_data;

int _dial(char *addr, int port)
{
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if (fd <0 ) return -1;
    struct sockaddr_in raddr;
    // set raddr
    memset(&raddr,0,sizeof(raddr));
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(port);
    inet_pton(AF_INET,addr,&raddr.sin_addr);
    // connect
    if(connect(fd,(struct sockaddr*)&raddr,sizeof(raddr)) < 0)
        return -1;
    return fd;
}

int _listen(char *addr, int port)
{
    int listenfd = socket(AF_INET,SOCK_STREAM,0);
    // set laddr
    struct sockaddr_in laddr;
    memset(&laddr,0,sizeof(laddr));
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(port);
    inet_pton(AF_INET,addr,&laddr.sin_addr);
    // bind and listen
    if (bind(listenfd,(struct sockaddr*)&laddr,sizeof(laddr)) < 0)
        return -1;
    if (listen(listenfd,5) < 0)
        return -1;
    return listenfd;
}

void *proxy(void *vargs)
{
    proxy_data *p_data = (proxy_data*)vargs;
    int srcfd = p_data->srcfd;
    int dstfd = p_data->dstfd;
    // char *info = p_data->info;
    // printf("proxy start %s %d->%d\n",info,srcfd,dstfd);
    char buf[2048] = {0};
    for (;;)
    {
        int n = recv(srcfd,buf,2048,0);
        if (n == 0) break;
        send(dstfd,buf,n,0);
    }
    // printf("proxy exit %s %d->%d\n",info,srcfd,dstfd);
    close(srcfd);
    pthread_exit(NULL);
}

void *handle(void *vsrcfd)
{
    int srcfd = *(int*)vsrcfd;
    char *dst_addr = DSTADDR;
    int dst_port = DSTPORT;
    int dstfd = _dial(dst_addr,dst_port);
    if (dstfd < 0)
    {
        printf("err\n");
        return NULL;
    }
    // init thread
    pthread_t fwd_pt, rev_pt;
    proxy_data fwd_data, rev_data;
    fwd_data.srcfd = rev_data.dstfd = srcfd;
    fwd_data.dstfd = rev_data.srcfd = dstfd;
    // fwd_data.info = "fwd"; rev_data.info = "rev";
    pthread_create(&fwd_pt,NULL,proxy,&fwd_data);
    pthread_create(&rev_pt,NULL,proxy,&rev_data);
    pthread_join(fwd_pt,NULL);
    pthread_join(rev_pt,NULL);
}

int main(int agrc, char **argv)
{
    char *addr = SRCADDR; int port = SRCPORT;
    int listenfd = _listen(addr,port);
    if (listenfd < 0) return -1;
    for (;;)
    {
        /* get conn addr
        struct sockaddr_in raddr;
        memset(&raddr,0,sizeof(raddr));
        char addr[INET_ADDRSTRLEN] = {0};
        socklen_t raddr_len = sizeof(raddr);
        int connfd = accept(listenfd,(struct sockaddr*)&raddr,&raddr_len);
        */
        int connfd = accept(listenfd,NULL,NULL);
        if (connfd < 0) continue;
        // printf("%d, from %s:%d\n",connfd,inet_ntop(AF_INET,&raddr.sin_addr,addr,INET_ADDRSTRLEN),ntohs(raddr.sin_port));
        /* muti-process
        int pid = fork();
        if (pid == 0)
        {
            close(listenfd);
            handle(connfd);
        }
        */ 
        // muti-thread
        pthread_t td;
        pthread_create(&td,NULL,handle,&connfd);
    }
    close(listenfd);
    return 0;
}

