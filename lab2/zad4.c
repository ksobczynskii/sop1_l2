#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),kill(0,SIGKILL),  exit(EXIT_FAILURE))

volatile sig_atomic_t received = 0;

ssize_t bulk_read(int fd, char* buf, size_t bytes)
{
    ssize_t c;
    ssize_t len =  0;
    while(bytes>0)
    {
        c = TEMP_FAILURE_RETRY(read(fd,buf,bytes));
        if(c == -1)
            return c;
        if(c==0)
            return len;
        bytes -= c;
        buf += c;
        len += c;

    }
    return len;

}

ssize_t bulk_write(int fd, char* buf, size_t bytes)
{
    ssize_t c;
    ssize_t len =0;
    while(bytes>0)
    {
        c = TEMP_FAILURE_RETRY(write(fd,buf,bytes));
        if(c<0)
            return c;
        bytes -= c;
        len += c;
        buf += c;
    }
    return len;
}
void child_work(int ms)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_DFL;

    if(-1==sigaction(SIGUSR1, &sa, NULL))
        ERR("sigaction");
    struct timespec ts = {0,ms*10000};
    while(true)
    {
        nanosleep(&ts,NULL);
        if(kill(getppid(), SIGUSR1))
            ERR("kill");
    }
}

void sig_action(int s)
{
    received++;
}

void config_actions()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sig_action;
    if(-1==sigaction(SIGUSR1, &sa, NULL) )
        ERR("sigaction");

}
void parent_work(int b, int s, char* name, pid_t kid_pid)
{
    char* buffer = (char*)malloc(s);
    if(!buffer)
        ERR("malloc");
    int fd = TEMP_FAILURE_RETRY(open(name,O_CREAT|  O_TRUNC|  O_WRONLY | O_APPEND,0777 ));
    int fd_urandom = TEMP_FAILURE_RETRY(open("/dev/urandom", O_RDONLY));
    if(fd <0 || fd_urandom <0)
        ERR("open");
    ssize_t size_read;
    for(int i=0; i<b; i++)
    {
        if((size_read = bulk_read(fd_urandom,buffer,s)) < 0)
            ERR("read");
        if((size_read = bulk_write(fd, buffer, size_read)) < 0)
            ERR("write");
        if(TEMP_FAILURE_RETRY(fprintf(stderr, "Read %ld bytes and received %d signals\n", size_read,received))<0)
            ERR("fprintf");
    }
    free(buffer);
    if(TEMP_FAILURE_RETRY(close(fd)))
        ERR("close");
    if(TEMP_FAILURE_RETRY(close(fd_urandom)))
        ERR("close");
    if(kill(kid_pid,SIGUSR2)) // zabijam dzieciaka o.o
        ERR("kill");
    while(wait(NULL)>0)
    {}

}
void make_kid(int ms, int b, int s, char* name)
{
    config_actions();
    pid_t kid_pid;
    switch((kid_pid = fork()))
    {
        case -1:
            ERR("fork");
        case 0:
            child_work(ms);
            break;
        default:
            parent_work( b,s,name, kid_pid);
            break;
    }
}


int main(int argc, char** argv)
{
    if(argc!=5)
    {
        fprintf(stderr, "Wrong nr. of arguments\n");
        exit(EXIT_FAILURE);
    }

    int ms = atoi(argv[1]);
    int b = atoi(argv[2]);
    int s = atoi(argv[3]);
    char* name = argv[4];
    make_kid(ms,b,s*0142*1024,name);
    return 0;
}