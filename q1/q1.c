//MAKE SURE STUDENT WITH LOWER INDEX GOES FORST IF BOTH ARRIVE AT SAME TIME 
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#define NA 0
#define WFW 1
#define WAL 2
#define UAL 3
#define WNG 4//washing

#define ANSI_RED "\033[1;31m"
#define ANSI_GREEN "\033[1;32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RESET "\x1b[0m"


unsigned int N;//No. of students
unsigned int M;//No. of washing machines
unsigned int ticks = 1;
sem_t mutex;
int count_no_wash = 0;

int tot_wait_time = 0;

void *thread_fn(void *);
//int num_of_active_wm_t = 0;

sem_t WM; //for washing machine
struct timespec start;

struct student{
    unsigned int T;//arrival time
    unsigned int W;//execution time: time taken to wash
    unsigned int P;//max waiting time (limit), or patience time
    unsigned int C;//current waiting time
    unsigned int status;//state of student: NOT_ARRIVED, WAITING_FOR_WASH, WASHED_AND_LEFT, UNWASHED_AND_LEFT. WASHING?

    
    unsigned int max_q_t; 
    unsigned int student_num;
    int wm_id;

    struct timespec begin_wash_time;

    pthread_t s_t;//each student is a thread
    pthread_mutex_t mutex_race;
    
    
} **stds;
pthread_mutex_t lock;
//washing machines are resources
struct washing_machine
{
    int w_id;
    
    int status;//0 is unoccupied, 1 is occupied
    pthread_mutex_t mutex;
    int student;
    
} **wms;

void update_time()
{
    sleep(2);
    ticks++;       
}

void main()
{
    //input
    scanf("%d %d", &N, &M);
    sem_init(&mutex, 0, 1);
    
    stds = (struct student **)malloc(sizeof(struct student *) * (N));
    wms = (struct washing_machine **)malloc(sizeof(struct washing_machine *) * (M));

    sem_init(&WM, 0, M);
    for(int i=0;i<M;i++)
    {
        wms[i] = malloc(sizeof(struct washing_machine));
        wms[i]->w_id = i;
        wms[i]->status = 0;
        wms[i]->student = -1;
        pthread_mutex_init(&(wms[i]->mutex),NULL); 
    }

    for(int i=0; i<N; i++)
    {
        stds[i] = malloc(sizeof(struct student));
        scanf("%d %d %d", &(stds[i]->T), &(stds[i]->W), &(stds[i]->P));
        stds[i]->student_num = i;
        stds[i]->max_q_t = stds[i]->T + stds[i]->W;
        stds[i]->wm_id = -1;
        stds[i]->status = NA;
        pthread_mutex_init(&(stds[i]->mutex_race),NULL);
        
    }

    clock_gettime(CLOCK_MONOTONIC, &start);	/* mark start time */
    
    //each student is a thread
    for(int i=0;i<N;i++)
    {
            pthread_create(&(stds[i]->s_t), NULL, thread_fn, (void *)stds[i]); 
            sleep(0.005);

    }
    for(int i=0;i<N;i++)
    {   
        pthread_join((stds[i]->s_t),NULL);
    }

    printf("%d\n", count_no_wash);
    printf("%d\n", tot_wait_time);

    float perc = (float)count_no_wash/(float)N * 100;
    if(perc >= 25)
        printf("Yes\n");
    else
        printf("No\n");

}

void *thread_fn(void *ptr)
{
    //Arrival
    struct student *st = (struct student *)ptr;

    sem_wait(&mutex);
    sleep(0.001);
    
    sem_post(&mutex);
    sleep(st->T);
    st->status = WFW;
    pthread_mutex_lock(&lock);//only one student arrives at an instant
    printf("%d: Student %d arrives\n", st->T, st->student_num+1);
    pthread_mutex_unlock(&lock);
    
    //Waiting for washing machine: try to lock semaphore for wm
    sleep(0.001);
    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME,&ts)==-1){
        perror("gettime ");
        return NULL;
    }
    ts.tv_sec += st->P;
    ts.tv_sec = ts.tv_sec + 1;//to make sure if washing machine is empty that second, student can use it
    ts.tv_nsec = 0;

    int ret;
    ret = sem_timedwait(&WM, &ts);
    
    //If student DID NOT get a washing machine
    pthread_mutex_lock(&st->mutex_race);
    if(ret == -1 && errno == ETIMEDOUT)//&& errno corresponds to time getting over
    {
        printf(ANSI_RED "%d: Student %d leaves without washing\n" ANSI_RESET, st->T+st->P, st->student_num+1);
        st->status = UAL;
        count_no_wash++;
        tot_wait_time = tot_wait_time + (st->P);
        pthread_mutex_unlock(&st->mutex_race);
        return NULL;
    }
    else if(ret == -1)
    {
        printf("Error in locking semaphore, exiting simulation\n");
        return NULL;
    }
    
    //else ret is 0, successfully locked washing machine, so now set status of wm, st and sleep
    
    clock_gettime(CLOCK_MONOTONIC, &(st->begin_wash_time));	/* mark point when student began washing clothes */
    int diff = (st->begin_wash_time.tv_sec - start.tv_sec); 
    //time student spent waiting
    tot_wait_time = tot_wait_time + (diff-st->T);

    pthread_mutex_unlock(&st->mutex_race);

    //Washing clothes
    for(int i=0;i<M;i++)
    {
        pthread_mutex_lock(&(wms[i]->mutex));
        if(wms[i]->status==0)
        {
            wms[i]->status = 0;
            wms[i]->student = st->student_num;
            st->wm_id = i;
            st->status = WNG;
            printf(ANSI_GREEN "%d: Student %d starts washing\n" ANSI_RESET, diff, st->student_num+1);//ticks??
            pthread_mutex_unlock(&(wms[i]->mutex));
            break;
        }
        pthread_mutex_unlock(&(wms[i]->mutex));
    }
    
    sleep(st->W);   //Washing  

    //finished washing clothes, update req statuses and unlock semaphore

    //num_of_active_wm_t--;
    printf(ANSI_YELLOW "%d: Student %d leaves after washing\n" ANSI_RESET, diff+st->W, st->student_num+1);
    st->status = WAL;

    pthread_mutex_lock(&(wms[st->wm_id]->mutex));
    wms[st->wm_id]->status = 0;
    wms[st->wm_id]->student = -1;
    pthread_mutex_unlock(&(wms[st->wm_id])->mutex);

    sem_post(&WM);

    return NULL;
    
}