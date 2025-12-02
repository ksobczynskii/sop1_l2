#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),kill(0,SIGKILL),  exit(EXIT_FAILURE))

volatile sig_atomic_t last_sig = 0;

void child_work(int ms, int n)
{
    int i=1;
    struct timespec ts = {0,ms*10000};
    int sent_usr2 = 0;
    while(true)
    {
        nanosleep(&ts,NULL);
        if(i==n)
        {
            if(kill(getppid(),SIGUSR2))
                ERR("kill");
            i=1;
            printf("[%d] Im a Child and I sent %d SIG_USR2's\n", getpid(), ++sent_usr2);
            fflush(stdout);
        }
        else
        {
            if(kill(getppid(), SIGUSR1))
                ERR("kill");
            i++;
        }
    }
}

void sig_action(int s)
{
    last_sig=s;
}

void config_actions()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sig_action;
    if(-1==sigaction(SIGUSR2, &sa, NULL) || -1==sigaction(SIGUSR1, &sa, NULL) )
        ERR("sigaction");

}
void parent_work(sigset_t oldmask)
{
    int count = 0;
    while(1)
    {
        last_sig = 0;
        while(last_sig!=SIGUSR2)
            sigsuspend(&oldmask);
        printf("[%d] Im a Parent and i Received %d SIGUSR2's\n", getpid(), ++count);
        fflush(stdout);
    }

}
void make_kid(int ms, int n)
{
    config_actions();
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    switch(fork())
    {
        case -1:
            ERR("fork");
        case 0:
            child_work(ms,n);
            break;
        default:
            parent_work(oldmask);
            break;
    }
}


int main(int argc, char** argv)
{
    if(argc!=3)
    {
        fprintf(stderr, "Wrong nr. of arguments\n");
        exit(EXIT_FAILURE);
    }

    int ms = atoi(argv[1]);
    int n = atoi(argv[2]);
    make_kid(ms,n);
}