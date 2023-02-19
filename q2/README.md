# Pizzeria

## Introduction

This problem is to simulate the functioning of a pizzeria. The customers enter, place their orders and wait for them to be completed/rejected/for all the chefs to leave. The chefs arrive and leave independently. They take orders that can be completed in their remaining time, use limited ingredients if required (and available), wait for ovens to be free and make the pizzas.  The simulation is to be done using POSIX threads.<br>
To compile : ``` gcc q2.c -lpthread```<br>

## Implementation

In the implementation, threads are maintained for chefs, customers and orders. 
<br>The inputs are read, the following structures are created to represent customers, chefs, and keep track of pizza types, limited ingredients, ovens and list of pizza orders. Global arrays are initialised.<br>

```c
struct chef 
{
    int id;
    pthread_t tid;
    int entry_time;
    int exit_time;
} **chefs;

struct customer //thread
{
    int id;
    pthread_t tid;
    int entry_time;
    int num_of_piz;
    int *piz_ids;
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
    int amt;  //initialized, and updates every time it is 
              //used.
    pthread_mutex_t mutex; //to ensure only 1 chef updates 
                           // this at a time
    

} **lim_ing_types;

struct oven 
{
    int id;
    pthread_mutex_t mutex; //to keep track of no. of pizzas  
                           //using an oven
    int status; //0 is unoccupied, 1 is occupied
} **ovens;

struct pizza_order
{
    int cust_id; //-1 for unassigned
    int piz_no; //index of pizza in cust->piz_ids array, -1 
                //for initialized/unassigned
    pthread_t t_id;
    int status;//initialize -1, 0 for rejected, 1 for 
               //accepted incomplete, 2 for complete
    pthread_mutex_t mutex;
} **orders;

```

Once the data structures are instantiated, start time of simulation is marked. <br>

```c
clock_gettime(CLOCK_MONOTONIC, &start);
```
Semaphores are maintained for pizza orders and ovens.

After input is completed, each customer is represented by starting a new thread which runs the function ```cust_fn```, each chef is represented byb creating a new thread that runs the function ```chef_fn```. Each of these functions simulates their respective entities as described:

### <b>Customer Function (cust_fn)</b>

#### <b>Customer Arrives</b>
Arrival is simulated by sleeping for the required time.


#### <b>Placing the order</b>
The order is placed immediately, and each pizza order is stored in an instance of the struct pizza_orders. A new thread is created to process each order, which calls the order_processing function.
```c
pthread_create(&(orders[i]->t_id), NULL, order_processing, (void *)orders[i]);

```

#### <b>Customer exit</b>
After placing their order, customer drives to the pick up spot and waits for their completed/rejected orders (i.e. waits for the order threads to join back). If all the orders are rejected, the customer leaves. This can happen in the middle of making the pizza, or as soon as the order is given.
Otherwise, the customer picks up their completed pizza and leaves.

### <b>Order Processing Function (order_processing)</b>

As soon as an order arrives for processing, the current limited ingredient amounts are checked to decide if the order can be accepted. If there aren't enough ingredients, it is immediately rejected. 
Otherwise, the pizza is added to the list of orders for the chef to pick from.

```sem_post(&PIZZA_ORDERS); ```

### <b>Chef Function (chef_fn)</b>

#### <b>Chef Arrives</b>
Arrival is simulated by sleeping for the required time.

#### <b>Try for an order</b>
The chef tries to lock the pizza_orders semaphore to accept an incomplete order and make it. If they don't get a pizza before their exit time, the chef leaves.
```c
ret = sem_timedwait(&PIZZA_ORDERS, &ts); //wait until chef has to leave
        if(ret == -1 && errno == ETIMEDOUT)
        {
            //chef has to leave
            printf(ANSI_BLUE "Chef %d exits at time %d.\n" ANSI_RESET, chf->id, chf->exit_time);
            return NULL;
        }
```
#### <b>Check for feasability</b>
Once they lock a pizza, they check if they have enough time to complete it using their exit time, and current time.
```c
clock_gettime(CLOCK_MONOTONIC, &(c_time));
        int curr_time = c_time.tv_sec - start.tv_sec;
        int remaining_time = chf->exit_time - curr_time;

        int piz_id = custs[orders[i]->cust_id-1]->piz_ids[orders[i]->piz_no];

        if(pz_types[piz_id-1]->prep_time<=remaining_time)
        {
            //Accept order!
            ....
        }
```
If they have enough time to complete it, they check whether they have enough limited ingredients to complete it. If they don't, the pizza is rejected at that point, after it's processing has started.
```c
for(int j=0; j<pz_types[piz_id-1]->num_lim_ing; j++)
            {
                int lim_ing_id = (pz_types[piz_id-1]->lim_ing_ids)[j];
                pthread_mutex_lock(&(lim_ing_types[lim_ing_id -1]->mutex));//unlock lim ings AFTER they have been put on pizza
                if(lim_ing_types[lim_ing_id -1]->amt == 0)
                {
                    printf(ANSI_BLUE "Chef %d could not complete pizza %d for order %d due to ingredient shortage." ANSI_RESET, chf->id, piz_id, orders[i]->cust_id);
                    orders[i]->status = 0;
                    printf(ANSI_RED "Order %d placed by customer %d partially processed and remaining couldn't be.\n" ANSI_RESET, orders[i]->cust_id, orders[i]->cust_id);

                    pthread_mutex_unlock(&(lim_ing_types[lim_ing_id -1]->mutex));
                    break;
                    //This pizza can't be made, mark it's status.
                    //go back to wait for new pizza
                }
            }
            pthread_mutex_unlock(&orders[i]->mutex);

```
#### <b>Try for an oven</b>
Once a pizza is ascertained as feasible, the chef immediately arranges the ingredients for 3 seconds, then releases ingredient mutex_lock, then tries for an oven by locking the ovens semaphore.
```c
ret_oven = sem_timedwait(&OVENS, &ts_oven); //wait until chef has to leave
                if(ret == -1 && errno == ETIMEDOUT)
                {
                    //chef has to leave
                    printf(ANSI_BLUE "Chef %d could not complete pizza %d of order %d because oven wasn't allocated yet.\n" ANSI_RESET,chf->id, piz_id, orders[i]->cust_id);
                    orders[i]->status = 0;//cancel this pizza order
                    printf(ANSI_RED "Order %d placed by customer %d partially processed and remaining couldn't be.\n" ANSI_RESET, orders[i]->cust_id, orders[i]->cust_id);

                    printf(ANSI_BLUE "Chef %d exits at time %d.\n" ANSI_RESET, chf->id, chf->exit_time);
                    return NULL;
                }
```
If they don't lock an oven before their exit time gets over, they reject the pizza and leave.

If they get an oven, they lock it and bake the pizza for the required time. Once complete, they mark it's status and try for a pizza again. 

**Assumption**: The preparation time entered for each pizza, is the time it takes to bake in the oven *after* it has been arranged for 3 seconds. (As per latest correction to question)

## Deadlocks

Deadlocks are avoided :
- A chef tries to lock the orders semaphore for a given amount of time *without holding any other resource*. 
- A chef can try to lock an oven semaphore only *after* locking an order semaphore. Therefore, a situation where one chef locks an order and waits for an oven, another locks an oven and waits for an order cannot arise.
- All semaphores are released once the entity has completed using the resource.