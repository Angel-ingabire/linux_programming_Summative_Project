/* =============================================================================
 * Project 4: Designing a Multithreaded Order Processing System in C
 * =============================================================================
 * File: order_processing.c
 * Description: Simulates an online food delivery order processing system
 *              using POSIX threads, mutex locks, and condition variables.
 *
 * System Components:
 *   1. Kitchen Thread (Producer) - Generates orders, adds to shared queue
 *   2. Delivery Thread (Consumer) - Removes orders from queue, processes delivery
 *   3. Monitoring Thread - Periodically reports system status
 *
 * Synchronization:
 *   - pthread_mutex_t: Protects shared queue and counters
 *   - pthread_cond_t: Coordinates producer-consumer (full/empty conditions)
 *
 * Queue Capacity: 5 orders (fixed-size shared buffer)
 *
 * Compilation:
 *   gcc -pthread -o order_processing order_processing.c
 *   ./order_processing
 *
 * Optional: ./order_processing <number_of_orders> (default: 20)
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* ===========================================================================
 * Constants and Configuration
 * ===========================================================================
 */
#define QUEUE_CAPACITY 5    /* Maximum orders in the shared queue */
#define DEFAULT_ORDERS 20   /* Default number of orders to process */
#define PREP_TIME_SEC 2     /* Time (seconds) for kitchen to prepare order */
#define DELIVERY_TIME_SEC 4 /* Time (seconds) for delivery processing */
#define MONITOR_INTERVAL 5  /* Time (seconds) between monitoring reports */

/* ===========================================================================
 * Shared Data Structures
 * ===========================================================================
 */

/* Order queue: circular buffer implementation
 *
 * Why circular buffer?
 *   - Efficient use of fixed-size array
 *   - No need to shift elements when adding/removing
 *   - O(1) enqueue and dequeue operations
 *   - Avoids dynamic memory allocation
 */
typedef struct
{
    int orders[QUEUE_CAPACITY]; /* Array storing order IDs */
    int head;                   /* Index for dequeue (front of queue) */
    int tail;                   /* Index for enqueue (back of queue) */
    int count;                  /* Current number of orders in queue */
} OrderQueue;

/* Global shared state (protected by mutex) */
OrderQueue queue = {.head = 0, .tail = 0, .count = 0};
int orders_prepared = 0;     /* Total orders prepared (producer) */
int orders_delivered = 0;    /* Total orders delivered (consumer) */
int total_orders_to_process; /* Number of orders before shutdown */
int program_running = 1;     /* Flag to signal threads to terminate */

/* Synchronization primitives */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;        /* Protects shared data */
pthread_cond_t cond_not_full = PTHREAD_COND_INITIALIZER;  /* Signal: queue not full */
pthread_cond_t cond_not_empty = PTHREAD_COND_INITIALIZER; /* Signal: queue not empty */

/* ===========================================================================
 * Queue Operations
 * ===========================================================================
 * These are internal helper functions called WITHIN the mutex lock.
 * They do NOT perform their own locking - the caller is responsible.
 * ===========================================================================
 */

/* Check if queue is full */
static inline int is_full(void)
{
    return queue.count >= QUEUE_CAPACITY;
}

/* Check if queue is empty */
static inline int is_empty(void)
{
    return queue.count <= 0;
}

/* Add an order to the queue (circular buffer)
 *
 * Parameters:
 *   order_id - The unique order ID to add
 *
 * Precondition: mutex is held, queue is NOT full
 */
static void enqueue_order(int order_id)
{
    queue.orders[queue.tail] = order_id;            /* Place order at tail */
    queue.tail = (queue.tail + 1) % QUEUE_CAPACITY; /* Advance tail (circular) */
    queue.count++;                                  /* Increment count */
}

/* Remove and return an order from the queue (circular buffer)
 *
 * Returns: The order ID that was removed
 *
 * Precondition: mutex is held, queue is NOT empty
 */
static int dequeue_order(void)
{
    int order_id = queue.orders[queue.head];        /* Get order from head */
    queue.head = (queue.head + 1) % QUEUE_CAPACITY; /* Advance head (circular) */
    queue.count--;                                  /* Decrement count */
    return order_id;
}

/* ===========================================================================
 * Kitchen Thread (Producer)
 * ===========================================================================
 *
 * Responsibilities:
 *   1. Generate new orders with unique incremental IDs
 *   2. Wait (2 seconds) simulating food preparation time
 *   3. Add completed order to the shared queue
 *   4. Wait when the queue is full (using condition variable)
 *   5. Signal delivery thread when new orders are available
 *
 * Synchronization pattern:
 *   - Lock mutex before accessing shared state
 *   - Wait on cond_not_full if queue is full (releases lock while waiting)
 *   - After adding order, signal cond_not_empty to wake consumer
 *   - Unlock mutex after critical section
 * ===========================================================================
 */
void *kitchen_thread(void *arg)
{
    int order_id = 0;
    (void)arg; /* Unused parameter */

    printf("[KITCHEN] Kitchen thread started. Preparing orders...\n");

    while (1)
    {
        /* Simulate food preparation time (2 seconds) */
        /* This is done OUTSIDE the lock to maximize concurrency */
        sleep(PREP_TIME_SEC);

        /* Create a new order (increment ID) */
        order_id++;

        /* ----- CRITICAL SECTION START (mutex locked) ----- */
        pthread_mutex_lock(&mutex);

        /* Check if we've processed all required orders */
        if (order_id > total_orders_to_process)
        {
            /* No more orders needed. But let any remaining orders be delivered */
            /* If queue is empty and all orders done, signal shutdown */
            if (is_empty())
            {
                program_running = 0;
                /* Wake up consumer and monitor in case they're waiting */
                pthread_cond_signal(&cond_not_empty);
                pthread_mutex_unlock(&mutex);
                break;
            }
            /* Otherwise, let remaining orders be delivered */
            pthread_mutex_unlock(&mutex);
            continue;
        }

        /* Wait if the queue is full (producer blocking)
         *
         * pthread_cond_wait atomically:
         *   1. Releases the mutex
         *   2. Puts this thread to sleep on cond_not_full
         *   3. When woken, re-acquires the mutex before returning
         *
         * This prevents busy-waiting and ensures we don't hold the lock
         * while sleeping.
         */
        while (is_full())
        {
            printf("[KITCHEN] Queue full! Waiting for delivery to free space...\n");
            pthread_cond_wait(&cond_not_full, &mutex);
        }

        /* Add the order to the shared queue */
        enqueue_order(order_id);
        orders_prepared++;
        printf("[KITCHEN] Order #%d prepared! Queue size: %d\n",
               order_id, queue.count);

        /* Signal the delivery thread that a new order is available */
        /* Signal is sent while holding the lock; the woken thread will
         * try to acquire the lock once we release it. */
        pthread_cond_signal(&cond_not_empty);

        /* ----- CRITICAL SECTION END (mutex unlocked) ----- */
        pthread_mutex_unlock(&mutex);
    }

    printf("[KITCHEN] All %d orders prepared. Kitchen shutting down.\n",
           total_orders_to_process);
    return NULL;
}

/* ===========================================================================
 * Delivery Thread (Consumer)
 * ===========================================================================
 *
 * Responsibilities:
 *   1. Remove orders from the shared queue
 *   2. Process each delivery (4 seconds)
 *   3. Wait when the queue is empty (using condition variable)
 *   4. Signal kitchen thread when space becomes available
 *
 * Synchronization pattern:
 *   - Lock mutex before accessing shared state
 *   - Wait on cond_not_empty if queue is empty (releases lock while waiting)
 *   - After removing order, signal cond_not_full to wake producer
 *   - Unlock mutex, then process delivery (outside lock)
 * ===========================================================================
 */
void *delivery_thread(void *arg)
{
    int order_id;
    (void)arg; /* Unused parameter */

    printf("[DELIVERY] Delivery thread started. Waiting for orders...\n");

    while (1)
    {
        /* ----- CRITICAL SECTION START (mutex locked) ----- */
        pthread_mutex_lock(&mutex);

        /* Wait if the queue is empty (consumer blocking)
         *
         * We also check if the program is shutting down:
         * If no orders remain and kitchen is done, terminate.
         */
        while (is_empty() && program_running)
        {
            printf("[DELIVERY] Queue empty! Waiting for kitchen to prepare orders...\n");
            pthread_cond_wait(&cond_not_empty, &mutex);
        }

        /* Check for shutdown condition:
         * Queue is empty AND kitchen is not producing more orders */
        if (is_empty() && !program_running)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        /* Remove an order from the queue */
        order_id = dequeue_order();
        printf("[DELIVERY] Order #%d picked up for delivery. Queue size: %d\n",
               order_id, queue.count);

        /* Signal the kitchen thread that space is available in the queue */
        pthread_cond_signal(&cond_not_full);

        /* ----- CRITICAL SECTION END (mutex unlocked) ----- */
        pthread_mutex_unlock(&mutex);

        /* Simulate delivery time (4 seconds)
         *
         * This is done OUTSIDE the mutex lock so the kitchen can
         * continue preparing and queuing orders during delivery.
         */
        sleep(DELIVERY_TIME_SEC);

        /* Update delivery count (mutex needed for shared variable) */
        pthread_mutex_lock(&mutex);
        orders_delivered++;
        printf("[DELIVERY] Order #%d delivered successfully!\n", order_id);
        pthread_mutex_unlock(&mutex);
    }

    printf("[DELIVERY] Delivery thread shutting down. %d orders delivered.\n",
           orders_delivered);
    return NULL;
}

/* ===========================================================================
 * Monitoring Thread
 * ===========================================================================
 *
 * Responsibilities:
 *   1. Periodically (every 5 seconds) display system status
 *   2. Reports: orders prepared, orders delivered, current queue size
 *   3. Safely accesses shared data with mutex protection
 *
 * Safety considerations:
 *   - Grabs the mutex only for the brief read operation
 *   - Releases the mutex immediately after reading
 *   - Does NOT hold the mutex while sleeping (no interference with
 *     producer-consumer operations)
 * ===========================================================================
 */
void *monitor_thread(void *arg)
{
    (void)arg; /* Unused parameter */

    printf("[MONITOR] Monitoring thread started. Reporting every %d seconds.\n",
           MONITOR_INTERVAL);

    while (1)
    {
        /* Sleep first so initial status doesn't interrupt startup */
        sleep(MONITOR_INTERVAL);

        /* ----- CRITICAL SECTION (brief read of shared state) ----- */
        pthread_mutex_lock(&mutex);

        /* Check if program is terminating */
        if (!program_running && is_empty())
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        /* Read and report current system status */
        printf("\n=== [MONITOR] System Status Report ===\n");
        printf("  Orders prepared:  %d\n", orders_prepared);
        printf("  Orders delivered: %d\n", orders_delivered);
        printf("  Current queue size: %d/%d\n", queue.count, QUEUE_CAPACITY);
        printf("========================================\n\n");

        /* ----- End of critical section ----- */
        pthread_mutex_unlock(&mutex);
    }

    printf("[MONITOR] Monitoring thread shutting down.\n");
    return NULL;
}

/* ===========================================================================
 * Main Function
 * ===========================================================================
 *
 * Sets up and launches all three threads, then waits for completion.
 * Handles command-line argument for number of orders to process.
 * ===========================================================================
 */
int main(int argc, char *argv[])
{
    pthread_t kitchen_tid, delivery_tid, monitor_tid;
    int ret;

    /* Determine number of orders to process */
    if (argc > 1)
    {
        total_orders_to_process = atoi(argv[1]);
        if (total_orders_to_process <= 0)
        {
            fprintf(stderr, "Invalid order count. Using default (%d).\n",
                    DEFAULT_ORDERS);
            total_orders_to_process = DEFAULT_ORDERS;
        }
    }
    else
    {
        total_orders_to_process = DEFAULT_ORDERS;
    }

    printf("=== Online Food Delivery Order Processing System ===\n");
    printf("Queue capacity: %d orders\n", QUEUE_CAPACITY);
    printf("Orders to process: %d\n", total_orders_to_process);
    printf("Preparation time: %ds, Delivery time: %ds\n\n",
           PREP_TIME_SEC, DELIVERY_TIME_SEC);

    /* Create the kitchen thread (producer)
     *
     * pthread_create parameters:
     *   1. tid pointer - populated with thread ID
     *   2. attr - NULL = default attributes
     *   3. start_routine - function to execute in new thread
     *   4. arg - argument passed to start_routine (NULL here)
     */
    ret = pthread_create(&kitchen_tid, NULL, kitchen_thread, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "Error creating kitchen thread: %d\n", ret);
        return 1;
    }

    /* Create the delivery thread (consumer) */
    ret = pthread_create(&delivery_tid, NULL, delivery_thread, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "Error creating delivery thread: %d\n", ret);
        return 1;
    }

    /* Create the monitoring thread */
    ret = pthread_create(&monitor_tid, NULL, monitor_thread, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "Error creating monitor thread: %d\n", ret);
        return 1;
    }

    /* Wait for all threads to finish
     *
     * pthread_join blocks until the specified thread terminates.
     * This is the main thread's way of waiting for workers.
     */
    pthread_join(kitchen_tid, NULL);
    pthread_join(delivery_tid, NULL);

    /* Signal monitor to stop and wait for it */
    /* (Monitor checks program_running flag; it may already be stopped) */
    pthread_join(monitor_tid, NULL);

    /* Print final summary */
    printf("\n=== Final Summary ===\n");
    printf("Total orders prepared:  %d\n", orders_prepared);
    printf("Total orders delivered: %d\n", orders_delivered);
    printf("=====================\n");

    /* Clean up synchronization primitives */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_not_full);
    pthread_cond_destroy(&cond_not_empty);

    return 0;
}
