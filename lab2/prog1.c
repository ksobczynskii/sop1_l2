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
#define MAX_FILE_NAME 100
volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t alarmed = 0;
ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;

    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;

        if (c == 0)
            return len;

        buf += c;
        len += c;
        count -= c;
    } while (count > 0);

    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;

    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;

        buf += c;
        len += c;
        count -= c;
    } while (count > 0);

    return len;
}
void set_sig(int sig, void (*func)(int))
{
    struct sigaction sa;
    memset(&sa,0,sizeof(struct sigaction));
    sa.sa_handler = func;
    sigaction(sig,&sa,NULL);
}
int generate_s()
{
    srand(getpid());
    return rand() %91 + 10;
}
void child_usr1(int sig)
{
    last_signal++;
}
void alarm_handler(int sig)
{
    alarmed = sig;
}

void wait_for_signal(int fd, char* buf, size_t buf_size)
{
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGUSR1);
    sigprocmask(SIG_BLOCK,&mask,&oldmask);
    while(alarmed==0)
    {
        sigsuspend(&oldmask);
        sig_atomic_t count = last_signal;
        for(int i=0; i<count;i++)
        {
            if(bulk_write(fd,buf,buf_size)==-1)
                ERR("write");
        }
        last_signal -= count;
    }
}
void child_work(char* n)
{
    set_sig(SIGUSR1,child_usr1);
    int s = generate_s();
    printf("[%d] I am a Child. n=%s, s = %d\n", getpid(), n, s);
    fflush(stdout);

    char file[MAX_FILE_NAME];
    sprintf(file,"%d",getpid());
    strcat(file,".txt");

    int fd = TEMP_FAILURE_RETRY(open(file, O_CREAT | O_WRONLY | O_TRUNC | O_APPEND, 0777));
    if(fd==-1)
        ERR("open");
    size_t buf_size = s*1024;
    char* block = (char*)malloc(buf_size);
    if(block==NULL)
        ERR("malloc");

    memset(block,n[0],buf_size);
    alarm(1);
    wait_for_signal(fd,block,buf_size);

    if(TEMP_FAILURE_RETRY(close(fd)==-1))
        ERR("close");
    free(block);
    exit(EXIT_SUCCESS);
}

void create_child(char* n)
{

    switch(fork())
    {
        case -1:
            ERR("fork");
        case 0:
            child_work(n);
            break;
        default:
            return;
    }
}



void parent_work()
{
    struct timespec ts = {0,10000};
    alarm(1);
    while(alarmed!=SIGALRM)
    {
        nanosleep(&ts,NULL);
        if(kill(0,SIGUSR1)==-1)
            ERR("kill");
    }
    while(true)
    {
        pid_t kid_pid = waitpid(0,NULL, WNOHANG);
        if(kid_pid ==-1)
        {
            if(errno == ECHILD)
            {
                return;
            }
            else{
                ERR("waitpid");
            }
        }
    }
}

int main(int argc, char** argv)
{
    set_sig(SIGUSR1,SIG_IGN);
    set_sig(SIGALRM,alarm_handler);
    for(int i=1; i<argc; i++)
    {
        create_child(argv[i]);
    }

    parent_work();
    return 0;
}