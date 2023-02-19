
/*in this version, orders, custs and chefs are threads
order_processing puts all orders which can be made with available limited ingredients into the list, i.e adds to semaphore count
orders which cannot be made are marked rejected in order_processing
when all order threads rejoin customer thread, customer checks them and leaves, or is rejected if none completed
chefs arrive , they look through orders semaphore for free orders if locked, 
semaphore is timed_wait, as they only wait until they have to leave
if they find a free order they check if they have enough time for it
if they have time they accept order, and check for ingredients
if they have ingredients they start prepping order, then enter queue for ovens, then finish pizza and go back to look for new pizza
they leave any time when the time gets over.

ASSUMPTION: PIZZA BAKES FOR T(PIZZA PREP TIME) SECONDS*/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#define ANSI_RED "\033[1;31m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RESET "\x1b[0m" 

//chef statuses
#define NOT_ARRIVED 0
#define WAITING_FOR_PIZZA_ORDER 1
#define PREPARING_PIZZA 2
#define WAITING_FOR_OVEN_ALLOC 3
#define LEFT 4

//ALL IDS ARE 1 INDEXED except pizza_no in pizza_order struct
struct timespec start;
//sem_t CHEFS;
sem_t PIZZA_ORDERS;
sem_t OVENS;
//entities

struct chef 
{
    int id;
    pthread_t tid;
    int entry_time;
    int exit_time;
    
    //int status; //Not arrived, Waiting for pizza order, preparing pizza,
                //waiting for oven allocation, left
    

} **chefs;

struct customer //thread
{
    int id;
    pthread_t tid;
    int entry_time;
    int num_of_piz;
    int *piz_ids;
    //int status; //not arrived, waiting for drive-thru alloc, arrived and ordered 
                //order rejected and exited, driving to pickup spot?, 
                //waiting at pickup spot, picked up and exited
    //order details(as customer places order immediately on arrival)
    //int *piz_statuses;//rejected = 0 or accepted=1, initialized to -1
    //int *chef_ids;// -2 for those pizzas which are rejected; -1 for all accepted and unassigned, -3 initialize
    //int order_status;//initialize -1, 0 for rejected(if all piz_statuses are rejected), 1 for accepted incomplete, 2 for complete
} **custs;

struct pizza
{
    int id;
    int prep_time;
    int num_lim_ing;
    int *lim_ing_ids;
} **pz_types;

struct limited_ingredients //resource
{
    int id;
    int amt;  //initialized, and updates every time it is used.
    pthread_mutex_t mutex; //to ensure only 1 chef updates this at a time
    

} **lim_ing_types;

struct oven //RESOURCE
{
    int id;
    pthread_mutex_t mutex; //to keep track of no. of pizzas using an oven
    int status;//0 is unoccupied, 1 is occupied
    //to keep track of order?
    //int cust_id;
    //int piz_id;
    //int piz_no; //index of pizza in cust->piz_ids array
    //int chef_id;

    
} **ovens;

struct pizza_order
{
    int cust_id; //-1 for unassigned
    int piz_no; //index of pizza in cust->piz_ids array, -1 for initialized/unassigned
    //int chef_id;// -2 for those pizzas which are rejected; -1 for all accepted and unassigned, -3 initialize
    
    pthread_t t_id;
    int status;//initialize -1, 0 for rejected, 1 for accepted incomplete, 2 for complete
    pthread_mutex_t mutex;
} **orders;


//global variables

int N, M, I, C, O, K, tot_no_of_orders;
int curr_order_num; //update by orders[i] only when locked
pthread_mutex_t order_count_lock;

void *cust_fn(void *);
void *chef_fn(void *);
void *order_processing(void *);
int last_chef_exit_time;

int main()
{
    // sem_init(&CHEFS, 0, 0);
    sem_init(&PIZZA_ORDERS, 0, 0);
    last_chef_exit_time = 0;
    tot_no_of_orders = 0;
    
    //input
    
    //number of chefs (n), the number of pizza varieties (m), 
    //the number of limited ingredients (i), the number of customers (c), the
    // number of ovens (o), and the time for a customer to reach the pickup spot (k).

    scanf("%d %d %d %d %d %d", &N, &M, &I, &C, &O, &K);
    
    chefs = (struct chef **)malloc(sizeof(struct chef *) * (N));
    custs = (struct customer **)malloc(sizeof(struct customer *) * (C));
    pz_types = (struct pizza **)malloc(sizeof(struct pizza *) * (M));
    lim_ing_types = (struct limited_ingredients **)malloc(sizeof(struct limited_ingredients *) * (I));
    ovens = (struct oven **)malloc(sizeof(struct oven *) * (O));
    
    sem_init(&OVENS, 0, O);
    
    //input pizza types
    for(int i=0; i<M; i++)
    {
        pz_types[i] = malloc(sizeof(struct pizza));
        scanf("%d %d %d", &(pz_types[i]->id), &(pz_types[i]->prep_time), &(pz_types[i]->num_lim_ing));
        pz_types[i]->lim_ing_ids = (int *)malloc(sizeof(int) * (pz_types[i]->num_lim_ing));
        
        for(int j=0; j<pz_types[i]->num_lim_ing; j++)
        {
            //pz_types[i]->lim_ing_ids[j] = malloc(sizeof(int));
            scanf("%d", &pz_types[i]->lim_ing_ids[j]);
            //scanf("%d", (pz_types[i]->lim_ing_ids+j*sizeof(int)));
        }
    }

    //input limited ingredient amounts
    for(int i=0; i<I; i++)
    {
        lim_ing_types[i] = malloc(sizeof(struct limited_ingredients));
        lim_ing_types[i]->id = i+1;
        scanf("%d", &(lim_ing_types[i]->amt));  //initialized
        pthread_mutex_init(&(lim_ing_types[i]->mutex),NULL); 
  
    }
    //initialise chef, input entry exit times
    for(int i=0; i<N; i++)
    {
        chefs[i] = malloc(sizeof(struct chef));
        
        chefs[i]->id = i+1;
        scanf("%d %d", &(chefs[i]->entry_time), &(chefs[i]->exit_time));
        if(last_chef_exit_time < chefs[i]->exit_time)
            last_chef_exit_time = chefs[i]->exit_time;
        //chefs[i]->status = NOT_ARRIVED; 
    }
    //input customers
    for(int i=0; i<C; i++)
    {
        custs[i] = malloc(sizeof(struct customer));
        scanf("%d %d", &(custs[i]->entry_time), &(custs[i]->num_of_piz));
        custs[i]->piz_ids = (int *)malloc(sizeof(int) * (custs[i]->num_of_piz));
        
        for(int j=0; j<custs[i]->num_of_piz; j++)
        {
            //custs[i]->piz_ids[j] = malloc(sizeof(int));
            scanf("%d", &custs[i]->piz_ids[j]);
        }
        custs[i]->id = i+1;
        //custs[i]->status = 0;//CHANGE THIS

        //initialise orders
        tot_no_of_orders = tot_no_of_orders + custs[i]->num_of_piz;
        //pizza_statuses : rejected = 0 or accepted=1, initialized to -1
        //custs[i]->piz_statuses = (int *)malloc(sizeof(int) * (custs[i]->num_of_piz));
        
        // for(int j=0; j<custs[i]->num_of_piz; j++)
        // {
        //     //custs[i]->piz_statuses[j] = malloc(sizeof(int));
        //     custs[i]->piz_statuses[j]=-1;
        // }
        //custs[i]->chef_ids = (int *)malloc(sizeof(int) * (custs[i]->num_of_piz));
        // for(int j=0; j<custs[i]->num_of_piz; j++)
        // {
        //     //custs[i]->chef_ids[j] = malloc(sizeof(int));
        //     custs[i]->chef_ids[j]=-1;
        // }
        //custs[i]->order_status = -1;//initialize -1, 0 for rejected(if all piz_statuses are rejected), 1 for accepted incomplete, 2 for complete
        
    }
    //we know total no. of pizzas all customers together will order, so create required space for orders
    orders = (struct pizza_order **)malloc(sizeof(struct pizza_order *) * (tot_no_of_orders));
    //initialize orders
    for(int i=0;i<tot_no_of_orders; i++)
    {
        orders[i] = malloc(sizeof(struct pizza_order));
        orders[i]->cust_id = -1;
        orders[i]-> piz_no = -1; //index of pizza in cust->piz_ids array
        //orders[i]->chef_id = -3; // -2 for those pizzas which are rejected; -1 for all accepted and unassigned, -3 initialize
        orders[i]->status = -1;//initialize -1, 0 for rejected, 1 for accepted incomplete, 2 for complete
        pthread_mutex_init(&(orders[i]->mutex),NULL); 
    }  
    //initialize ovens
    for(int i=0;i<O; i++)
    {
        ovens[i] = malloc(sizeof(struct oven));
        ovens[i]->id = i+1;
        pthread_mutex_init(&(ovens[i]->mutex),NULL); 
        ovens[i]->status = 0;//0 is unoccupied, 1 is occupied
    //to keep track of order?
        // ovens[i]->cust_id = -1;
        // ovens[i]->piz_id = -1;
        // ovens[i]->piz_no = -1; //index of pizza in cust->piz_ids array
        // ovens[i]->chef_id = -1;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);	/* mark start time */
    
    pthread_mutex_init(&(order_count_lock),NULL); 

    curr_order_num = 0;//index the list of orders at 0, i.e. head of queue is 0
    
    //Start sim by creating all chef threads, Create all customer threads
    printf("Simulation started\n");
    for(int chefi=0; chefi<N; chefi++)
    {
        pthread_create(&(chefs[chefi]->tid), NULL, chef_fn, (void*)(chefs[chefi]));
    }
    for(int custi = 0; custi<C; custi++)
    {
        pthread_create(&(custs[custi]->tid), NULL, cust_fn, (void*)(custs[custi]));
    }
    
    for(int chefi=0; chefi<N; chefi++)
    {
        pthread_join(chefs[chefi]->tid, NULL);
    }
    for(int custi = 0; custi<C; custi++)
    {
        pthread_join(custs[custi]->tid, NULL);
    }
    //DESTROY MUTEXES AND SEMAPHORES?
    // pthread_mutex_destroy(&leftMut);
    // for(i=0;i<n;i++){
    //     pthread_mutex_destroy(&compMut[i]);
    // }
    printf("Simulation ended\n");  

}

void *chef_fn(void *arg)
{
    struct chef *chf = (struct chef *)arg;
    struct timespec ts_check;
    //Arrival
    sleep(chf->entry_time);
    //chf->status = WAITING_FOR_PIZZA_ORDER;
    //sem_post(&CHEFS);//inc no. of available chefs
    printf(ANSI_BLUE "Chef %d arrived at time %d.\n" ANSI_RESET, chf->id, chf->entry_time);

    while(1)
    {
        //first check if curr_time == chef exit time, leave if true
        
        clock_gettime(CLOCK_MONOTONIC, &(ts_check));
        int time_now = ts_check.tv_sec - start.tv_sec;
        if(time_now == chf->exit_time)
        {
            //chef has to leave
            printf(ANSI_BLUE "Chef %d exits at time %d.\n" ANSI_RESET, chf->id, chf->exit_time);
            return NULL;
        }

        //chef tries for orders semaphore
        struct timespec ts;
        if(clock_gettime(CLOCK_REALTIME,&ts)==-1){
            perror("gettime ");
            return NULL;
        }
        ts.tv_sec += (chf->exit_time-time_now);
        ts.tv_nsec = 0;

        int ret;
        ret = sem_timedwait(&PIZZA_ORDERS, &ts); //wait until chef has to leave
        if(ret == -1 && errno == ETIMEDOUT)
        {
            //chef has to leave
            printf(ANSI_BLUE "Chef %d exits at time %d.\n" ANSI_RESET, chf->id, chf->exit_time);
            return NULL;
        }

        //loop through order list to find the order, now that semaphore pizza_orders has been locked.
        int i;//index of free order
        for(i=0; i<curr_order_num; i++)
        {
            pthread_mutex_lock(&orders[i]->mutex);
            if(orders[i]->status == -1) //initialized only
            {
                break;
            }
            pthread_mutex_unlock(&orders[i]->mutex); //for all non free orders

        }
        
        int enough_time = 0;
        //check if chef can complete the order
        struct timespec c_time;
        clock_gettime(CLOCK_MONOTONIC, &(c_time));
        int curr_time = c_time.tv_sec - start.tv_sec;
        int remaining_time = chf->exit_time - curr_time;

        int piz_id = custs[orders[i]->cust_id-1]->piz_ids[orders[i]->piz_no];

        if(pz_types[piz_id-1]->prep_time<=remaining_time)
        {
            //Accept order!
            printf(ANSI_RED "Pizza %d in order %d has been assigned to Chef %d.\n" ANSI_RESET, piz_id, orders[i]->cust_id, chf->id);
            printf(ANSI_RED "Order %d placed by customer %d is being processed.\n" ANSI_RESET, orders[i]->cust_id, orders[i]->cust_id);
            orders[i]->status = 1;
            //check if there are ingredients
            for(int j=0; j<pz_types[piz_id-1]->num_lim_ing; j++)
            {
                int lim_ing_id = (pz_types[piz_id-1]->lim_ing_ids)[j];
                pthread_mutex_lock(&(lim_ing_types[lim_ing_id -1]->mutex));//unlock lim ings AFTER they have been put on pizza
                if(lim_ing_types[lim_ing_id -1]->amt == 0)
                {
                    printf(ANSI_BLUE "Chef %d could not complete pizza %d for order %d due to ingredient shortage.\n" ANSI_RESET, chf->id, piz_id, orders[i]->cust_id);
                    printf(ANSI_RED "Order %d placed by customer %d partially processed and remaining couldn't be.\n" ANSI_RESET, orders[i]->cust_id, orders[i]->cust_id);
                    orders[i]->status = 0;
                    pthread_mutex_unlock(&(lim_ing_types[lim_ing_id -1]->mutex));
                    break;
                    //This pizza can't be made, mark it's status.
                    //go back to wait for new pizza
                }
            }
            pthread_mutex_unlock(&orders[i]->mutex);

            if(orders[i]->status !=0) //there are enough ingredients for pizza
            {
                //pizza can be made 
                //arrange
                for(int j=0; j<pz_types[piz_id-1]->num_lim_ing; j++)
                {
                    int lim_ing_id = (pz_types[piz_id-1]->lim_ing_ids)[j];
                    lim_ing_types[lim_ing_id -1]->amt --;
                    pthread_mutex_unlock(&(lim_ing_types[lim_ing_id -1]->mutex));
                }//unlock lim ings AFTER they have been put on pizza

                printf(ANSI_BLUE "Chef %d is preparing the pizza %d from order %d.\n" ANSI_RESET, chf->id, piz_id, orders[i]->cust_id);
                sleep(3);
                
                //try for oven
                printf(ANSI_BLUE "Chef %d is waiting for oven allocation for pizza %d from order %d.\n" ANSI_RESET, chf->id, piz_id, orders[i]->cust_id);

                struct timespec c_time_o;
                clock_gettime(CLOCK_MONOTONIC, &(c_time_o));
                int curr_time_o = c_time_o.tv_sec - start.tv_sec;
                
                struct timespec ts_oven;
                if(clock_gettime(CLOCK_REALTIME,&ts_oven)==-1){
                    perror("gettime ");
                    return NULL;
                }
                ts_oven.tv_sec += (chf->exit_time-curr_time_o);
                ts_oven.tv_nsec = 0;

                int ret_oven;
                ret_oven = sem_timedwait(&OVENS, &ts_oven); //wait until chef has to leave
                if(ret == -1 && errno == ETIMEDOUT)
                {
                    //chef has to leave
                    printf(ANSI_BLUE "Chef %d could not complete pizza %d of order %d because oven wasn't allocated before exit time.\n" ANSI_RESET,chf->id, piz_id, orders[i]->cust_id);
                    orders[i]->status = 0;//cancel this pizza order
                    printf(ANSI_BLUE "Chef %d exits at time %d.\n" ANSI_RESET, chf->id, chf->exit_time);
                    printf(ANSI_RED "Order %d placed by customer %d partially processed and remaining couldn't be.\n" ANSI_RESET, orders[i]->cust_id, orders[i]->cust_id);
                    
                    return NULL;
                }

                //loop through oven list to find the oven
                int j;//index of free oven
                for(j=0; j<O; j++)
                {
                    pthread_mutex_lock(&ovens[j]->mutex);
                    if(ovens[j]->status == 0) //unoccupied
                    {
                        ovens[j]->status = 1;//mark occupied
                        pthread_mutex_unlock(&ovens[j]->mutex);

                        break;
                    }
                    pthread_mutex_unlock(&ovens[j]->mutex);

                }
                //use oven to bake pizza IF there's enough time left
                struct timespec c_time_1;
                clock_gettime(CLOCK_MONOTONIC, &(c_time_1));
                int curr_time_1 = c_time_1.tv_sec - start.tv_sec;
                //if(curr_time_1 + (pz_types[piz_id-1]->prep_time-3) >= chf->exit_time)
                if(curr_time_1 + (pz_types[piz_id-1]->prep_time) >= chf->exit_time)
                {
                    //chef will have to leave so cancel order
                    printf(ANSI_BLUE "Chef %d could not complete pizza %d of order %d because oven was allocated too late.\n" ANSI_RESET,chf->id, piz_id, orders[i]->cust_id);
                    orders[i]->status = 0;
                    printf(ANSI_RED "Order %d placed by customer %d partially processed and remaining couldn't be.\n" ANSI_RESET, orders[i]->cust_id, orders[i]->cust_id);

                    sem_post(&OVENS);//release oven
                    //go back to sem_timedwait for new order in case there's an order that can be completed
                }
                else
                {
                    //bake pizza for not t-3 secs, but t secs
                    printf(ANSI_BLUE "Chef %d has put the pizza %d for order %d in oven %d at time %d.\n" ANSI_RESET,chf->id, piz_id, orders[i]->cust_id, j+1, curr_time_1);
                    sleep(pz_types[piz_id-1]->prep_time);
                    sem_post(&OVENS);//release oven 
                    //find current time again
                    curr_time_1 = curr_time_1 + pz_types[piz_id-1]->prep_time;
                    printf(ANSI_BLUE "Chef %d has picked up the pizza %d for order %d from the oven %d at time %d.\n" ANSI_RESET,chf->id, piz_id, orders[i]->cust_id, j+1, curr_time_1);
                    orders[i]->status = 2;
                    //chef finished pizza, go back and try for another
                }

            }
        }
        else//there's not enough time to make this pizza
        {
            pthread_mutex_unlock(&orders[i]->mutex);
            //release this order
            sem_post(&PIZZA_ORDERS);
            //go back and wait for new order
        }
    }    
}


void *cust_fn(void *arg)
{
    struct customer *cst = (struct customer *)arg;
    //Arrival 
    sleep(cst->entry_time);
    //cst->status = NOT_ARRIVED;
    printf(ANSI_YELLOW "Customer %d arrives at time %d.\n" ANSI_RESET, cst->id, cst->entry_time);
    printf(ANSI_YELLOW "Customer %d is waiting for the drive-thru allocation.\n" ANSI_RESET, cst->id);
    printf(ANSI_YELLOW "Customer %d enters the drive-thru zone and gives out their order %d.\n" ANSI_RESET, cst->id, cst->id);
    printf(ANSI_RED "Order %d placed by customer %d has pizzas {" ANSI_RESET,cst->id, cst->id);

    for(int j=0; j<cst->num_of_piz; j++)
    {
        if(j==cst->num_of_piz-1)
            printf(ANSI_RED "%d}.\n" ANSI_RESET, cst->piz_ids[j]);
        else
            printf(ANSI_RED "%d,"ANSI_RESET, cst->piz_ids[j]);
    }
    //customer order has been placed
    printf(ANSI_RED "Order %d placed by customer %d awaits processing.\n" ANSI_RESET, cst->id, cst->id);

    //parse the order to divide it into individual pizza orders,
    pthread_mutex_lock(&order_count_lock);
    
    int start_index = curr_order_num;
    int index;
    for(int i=0; i<cst->num_of_piz; i++)
    {
        //create an order entry for each pizza
        index = curr_order_num;
        curr_order_num++;
        pthread_mutex_lock(&orders[index]->mutex);
        orders[index]->cust_id = cst->id;
        orders[index]-> piz_no = i; //index of pizza in cust->piz_ids array
        pthread_mutex_unlock(&orders[index]->mutex);

    }
    pthread_mutex_unlock(&order_count_lock);
    int end_index = index;
    
    // and create threads for each order to call the order_processing fn.
    //printf("EXTRA: For customer %d, their pizza orders are stored in index %d to %d of orders array\n", cst->id, start_index, end_index);
    for(int i=start_index; i<=end_index; i++)
    {
        pthread_create(&(orders[i]->t_id), NULL, order_processing, (void *)orders[i]);
    }
    //after orders are created, customer drives to pick up point: ie.e sleep k secoonds
    sleep(K);
    printf(ANSI_YELLOW "Customer %d is waiting at the pickup spot.\n" ANSI_RESET, cst->id);
    
    int at_least_one = 0;
    for(int i=start_index; i<=end_index; i++)
    {
        pthread_join(orders[i]->t_id,NULL);
        if(orders[i]->status == 2)
        {
            //printf(ANSI_YELLOW "Customer %d picks up their pizza %d.\n" ANSI_RESET, orders[i]->cust_id, cst->piz_ids[orders[i]->piz_no]);
            at_least_one = 1;
        }

    }
    //once all order threads are complete, check each order status, if all are rejected, reject customer, else leave.
    
    
    // for(int i=start_index; i<=end_index; i++)
    // {
    //     if(orders[i]->status == 2)
    //     {
    //         printf(ANSI_YELLOW "Customer %d picks up their pizza %d.\n" ANSI_RESET, orders[i]->cust_id, cst->piz_ids[orders[i]->piz_no]);
    //         at_least_one = 1;
    //     }
    // }
    if(at_least_one == 0)
        printf(ANSI_YELLOW "Customer %d rejected.\n" ANSI_RESET,cst->id);
    
    printf(ANSI_YELLOW "Customer %d exits the drive thru zone.\n" ANSI_RESET, cst->id);
}
void *order_processing(void *arg)
{
    struct pizza_order *order = (struct pizza_order *)arg;
    //process order to accept, reject, or wait

    //if lim_ings, 
    int piz_id = custs[order->cust_id-1]->piz_ids[order->piz_no]; //index of pizza in cust->piz_ids array
    
    for(int j=0; j<pz_types[piz_id-1]->num_lim_ing; j++)
    {
        int lim_ing_id = (pz_types[piz_id-1]->lim_ing_ids)[j];
        pthread_mutex_lock(&(lim_ing_types[lim_ing_id -1]->mutex));
        if(lim_ing_types[lim_ing_id -1]->amt == 0)
        {
            //order->chef_id = -2; // -2 for those pizzas which are rejected; -1 for all accepted and unassigned, -3 initialize
            order->status = 0;//initialize -1, 0 for rejected, 1 for accepted incomplete, 2 for complete
            pthread_mutex_unlock(&(lim_ing_types[lim_ing_id -1]->mutex));

            return NULL;
            //This pizza can't be made, mark it's status.
        }
        pthread_mutex_unlock(&(lim_ing_types[lim_ing_id -1]->mutex));

    }
    sem_post(&PIZZA_ORDERS);//inc pizza orders that can be fulfilled by a chef
    
    //int piz_id = custs[order->cust_id-1]->piz_ids[order->piz_no]; //index of pizza in cust->piz_ids array
    //sleep(pz_types[piz_id-1]->prep_time);

    while(order->status!=0 && order->status!=2)
    {
        struct timespec c_time;
        clock_gettime(CLOCK_MONOTONIC, &(c_time));
        int curr_time = c_time.tv_sec - start.tv_sec;
        if(curr_time > last_chef_exit_time)
        { //if all chefs are gone and order isn't rejected or complete
            order->status = 0;
            printf(ANSI_RED "Order %d placed by customer %d completely rejected.\n" ANSI_RESET, order->cust_id, order->cust_id);
            return NULL;
        }
    }    
    if(order->status == 2)
    {
        printf(ANSI_RED "Order %d placed by customer %d has been processed.\n" ANSI_RESET, order->cust_id, order->cust_id);
        printf(ANSI_YELLOW "Customer %d picks up their pizza %d.\n" ANSI_RESET, order->cust_id, custs[order->cust_id-1]->piz_ids[order->piz_no]);
    }
    return NULL;
}


    
    