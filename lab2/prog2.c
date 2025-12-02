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
#include <stddef.h>
#define MAX_BUF_LEN 4096
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),kill(0,SIGKILL),  exit(EXIT_FAILURE))
volatile sig_atomic_t got_sick = 0;
volatile sig_atomic_t sickness_prob = 0;
volatile sig_atomic_t coughed = 0;
void set_sig(int sig, void (*func)(int))
{
    struct sigaction sa;
    memset(&sa,0,sizeof(struct sigaction));
    sa.sa_handler = func;
    sigaction(sig,&sa,NULL);
}
void set_sig_adv(int sig, void (*func)(int,siginfo_t*, void*))
{
    struct sigaction sa;
    memset(&sa,0,sizeof(struct sigaction));
    sa.sa_sigaction = func;
    sa.sa_flags = SA_SIGINFO;
    sigaction(sig,&sa,NULL);
}
void usage(char *name)
{
    fprintf(stderr, "USAGE: wrong argument, %s\n", name);
    exit(EXIT_FAILURE);
}

void child_exit_notify(int sig)
{
    printf("Child[%d] exits with %d\n",getpid(),coughed);
    exit(coughed);
}

void wait_out_kids()
{

    while(1)
    {
        errno=0;
        int status;
        pid_t kid_pid = waitpid(0,&status,WNOHANG);
        if(kid_pid==-1)
        {
            if(errno==ECHILD)
                return;
            else
                ERR("waitpid");
        }
    }
}
void get_coughed_at(int sig, siginfo_t* info, void* useless)
{
    if(got_sick)
        return;
    pid_t who_coughed;
    if(info == NULL)
        who_coughed =-1;
    else{
        who_coughed = info->si_pid;
    }

    printf("Child[%d]: %d has coughed at me!\n",getpid(),who_coughed);
    bool sick = rand() %101 <= sickness_prob;
    if(sick)
        got_sick = 1;

}
void mom_pickup(int sig)
{
    printf("Child[%d] exits with %d\n",getpid(),coughed);
    exit(coughed);
}
void kid_work(int is_ill,int k)
{
    srand(getpid());
    got_sick = is_ill;
    set_sig(SIGTERM,child_exit_notify);
    printf("Child[%d] starts day in the kindergarten, ill: %d\n",getpid(), is_ill);
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGUSR1);
    sigprocmask(SIG_BLOCK,&mask,&oldmask);
    while(got_sick==0)
    {
        sigsuspend(&oldmask);
    }
    printf("Child[%d] get sick!\n",getpid());
    set_sig(SIGALRM,mom_pickup);
    alarm(k);
    while(true)
    {
        int ms = rand()%151 + 50;
        struct timespec ts = {0,1000000*ms};
        struct timespec ts2 = {2,0}; // usun
        nanosleep(&ts2,NULL);
        if(kill(0,SIGUSR1)==-1)
            ERR("kill");
        coughed++;
        printf("Child[%d] is coughing %d\n",getpid(),coughed);
    }

}
void terminate_kids(int sig)
{
    if(kill(0,SIGTERM)==-1)
        ERR("kill");
}
void send_signals(int t)
{
    set_sig(SIGALRM,terminate_kids);
    alarm(t);
}
void parent_work(int t)
{
    send_signals(t);
    wait_out_kids();
}
void make_kids(int n, int k)
{
    pid_t kid_pid = fork();
    switch (kid_pid) {
        case -1:
            ERR("fork");
        case 0:
            kid_work(1,k);
            break;
        default:
            break;
    }

    for(int i=1; i<n; i++)
    {
        kid_pid = fork();
        switch (kid_pid) {
            case -1:
                ERR("fork");
            case 0:
                kid_work(0,k);
                break;
            default:
                break;
        }
    }
}


int main(int argc, char** argv)
{
    if(argc!=5)
    {
        usage("argv");
    }
    int t, k, n, p;
    t = atoi(argv[1]);
    k = atoi(argv[2]);
    n = atoi(argv[3]);
    p = atoi(argv[4]);
    if(t>100||t<1)
        usage("t");
    if(k>100||k<1)
        usage("k");
    if(n>30 || n<1)
        usage("n");
    if(p>100 || p<1)
        usage("p");
    sickness_prob = p;
    set_sig_adv(SIGUSR1, get_coughed_at);
    set_sig(SIGTERM,SIG_IGN);
    printf("KG[%d]: Alarm has been set for %d sec\n",getpid(),t);
    make_kids(n,k);
    set_sig(SIGUSR1,SIG_IGN);
    parent_work(t);
    printf("KG[%d]: Simulation has ended!\n", getpid());
}