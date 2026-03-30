#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* ============================================================
 * Constants
 * ============================================================ */
#define BELT_CAPACITY       5
#define TOTAL_LUGGAGE       20
#define LOADER_DELAY        2      // seconds per luggage
#define AIRCRAFT_DELAY      4      // seconds per luggage
#define MONITOR_DELAY       5      // seconds between reports

/* ============================================================
 * struct shared_data - shared data for baggage synchronization
 * @belt: storage array for luggage IDs (circular buffer)
 * @head: index for removing luggage (consumer)
 * @tail: index for adding luggage (producer)
 * @count: current number of items on the belt
 * @loaded: total number of items loaded (produced)
 * @dispatched: total number of items removed (consumed)
 * @running: flag to control thread execution
 * @mutex: mutex lock for safe access
 * @c_space: condition for available space (belt not full)
 * @c_items: condition for available items (belt not empty)
 */
struct shared_data
{
    int belt[BELT_CAPACITY];
    int head;
    int tail;
    int count;
    int loaded;
    int dispatched;
    int running;
    pthread_mutex_t mutex;
    pthread_cond_t c_space;
    pthread_cond_t c_items;
};

/* ============================================================
 * Function: init_shared_data()
 * Initializes all shared data structures
 * ============================================================ */
void init_shared_data(struct shared_data *d)
{
    memset(d, 0, sizeof(struct shared_data));
    d->head = 0;
    d->tail = 0;
    d->count = 0;
    d->loaded = 0;
    d->dispatched = 0;
    d->running = 1;
    pthread_mutex_init(&d->mutex, NULL);
    pthread_cond_init(&d->c_space, NULL);
    pthread_cond_init(&d->c_items, NULL);
}

/* ============================================================
 * Function: destroy_shared_data()
 * Cleans up synchronization primitives
 * ============================================================ */
void destroy_shared_data(struct shared_data *d)
{
    pthread_mutex_destroy(&d->mutex);
    pthread_cond_destroy(&d->c_space);
    pthread_cond_destroy(&d->c_items);
}

/* ============================================================
 * Function: monitor()
 * Monitoring thread that runs every 5 seconds
 * Safely reads shared variables and prints system status
 * @arg: Pointer to shared data struct
 * Return: NULL
 */
void *monitor(void *arg)
{
    struct shared_data *d = (struct shared_data *)arg;
    int last_loaded = 0;
    int last_dispatched = 0;

    while (d->running && (d->dispatched < TOTAL_LUGGAGE))
    {
        sleep(MONITOR_DELAY);
        
        pthread_mutex_lock(&d->mutex);
        
        int loaded = d->loaded;
        int dispatched = d->dispatched;
        int count = d->count;
        
        pthread_mutex_unlock(&d->mutex);
        
        /* Calculate throughput since last report */
        int loaded_delta = loaded - last_loaded;
        int dispatched_delta = dispatched - last_dispatched;
        
        printf("\n========================================\n");
        printf("        MONITOR REPORT\n");
        printf("========================================\n");
        printf("Total loaded to belt:   %d\n", loaded);
        printf("Total dispatched:        %d\n", dispatched);
        printf("Current belt size:       %d/%d\n", count, BELT_CAPACITY);
        printf("Throughput (last %ds):\n", MONITOR_DELAY);
        printf("  - Loaded:  %d items\n", loaded_delta);
        printf("  - Dispatched: %d items\n", dispatched_delta);
        printf("========================================\n\n");
        
        last_loaded = loaded;
        last_dispatched = dispatched;
        
        /* Check if processing complete */
        if (dispatched >= TOTAL_LUGGAGE)
        {
            pthread_mutex_lock(&d->mutex);
            d->running = 0;
            pthread_cond_signal(&d->c_space);
            pthread_cond_signal(&d->c_items);
            pthread_mutex_unlock(&d->mutex);
        }
    }
    
    printf("[MONITOR] Monitoring thread terminating.\n");
    return (NULL);
}

/* ============================================================
 * Function: loader()
 * Producer thread for luggage (2s delay)
 * Loads luggage onto conveyor belt
 * @arg: Pointer to shared data struct
 * Return: NULL
 */
void *loader(void *arg)
{
    struct shared_data *d = (struct shared_data *)arg;
    int luggage_id = 1;

    while (d->running && (luggage_id <= TOTAL_LUGGAGE))
    {
        sleep(LOADER_DELAY);
        
        pthread_mutex_lock(&d->mutex);
        
        /* Wait while belt is full */
        while (d->count == BELT_CAPACITY && d->running)
        {
            printf("[LOADER] Belt FULL! Waiting for space...\n");
            pthread_cond_wait(&d->c_space, &d->mutex);
        }
        
        /* Check if thread should exit */
        if (!d->running || luggage_id > TOTAL_LUGGAGE)
        {
            pthread_mutex_unlock(&d->mutex);
            break;
        }
        
        /* Add luggage to circular buffer */
        d->belt[d->tail] = luggage_id;
        d->tail = (d->tail + 1) % BELT_CAPACITY;
        d->count++;
        d->loaded++;
        
        printf("[LOADER] Luggage #%d on belt | Belt size: %d/%d\n", 
               luggage_id, d->count, BELT_CAPACITY);
        
        /* Signal aircraft that belt has items */
        pthread_cond_signal(&d->c_items);
        
        pthread_mutex_unlock(&d->mutex);
        
        luggage_id++;
    }
    
    printf("[LOADER] Production complete. Total loaded: %d\n", d->loaded);
    return (NULL);
}

/* ============================================================
 * Function: aircraft()
 * Consumer thread for luggage (4s delay)
 * Removes luggage from belt and loads onto aircraft
 * @arg: Pointer to shared data struct
 * Return: NULL
 */
void *aircraft(void *arg)
{
    struct shared_data *d = (struct shared_data *)arg;

    while (d->running && (d->dispatched < TOTAL_LUGGAGE))
    {
        sleep(AIRCRAFT_DELAY);
        
        pthread_mutex_lock(&d->mutex);
        
        /* Wait while belt is empty */
        while (d->count == 0 && d->running)
        {
            if (d->loaded >= TOTAL_LUGGAGE)
            {
                /* No more luggage coming */
                pthread_mutex_unlock(&d->mutex);
                printf("[AIRCRAFT] No more luggage to process.\n");
                return (NULL);
            }
            printf("[AIRCRAFT] Belt EMPTY! Waiting for luggage...\n");
            pthread_cond_wait(&d->c_items, &d->mutex);
        }
        
        /* Check if thread should exit */
        if (!d->running || d->dispatched >= TOTAL_LUGGAGE)
        {
            pthread_mutex_unlock(&d->mutex);
            break;
        }
        
        /* Remove luggage from circular buffer */
        int luggage_id = d->belt[d->head];
        d->head = (d->head + 1) % BELT_CAPACITY;
        d->count--;
        d->dispatched++;
        
        printf("[AIRCRAFT] Luggage #%d dispatched | Belt size: %d/%d\n", 
               luggage_id, d->count, BELT_CAPACITY);
        
        /* Signal loader that belt has space */
        pthread_cond_signal(&d->c_space);
        
        pthread_mutex_unlock(&d->mutex);
    }
    
    printf("[AIRCRAFT] Dispatch complete. Total dispatched: %d\n", d->dispatched);
    return (NULL);
}

/* ============================================================
 * Function: print_belt_contents()
 * Utility to display current belt contents (debug only)
 * ============================================================ */
void print_belt_contents(struct shared_data *d)
{
    pthread_mutex_lock(&d->mutex);
    
    printf("[DEBUG] Belt: [");
    for (int i = 0; i < d->count; i++)
    {
        int idx = (d->head + i) % BELT_CAPACITY;
        printf(" %d", d->belt[idx]);
    }
    printf(" ]\n");
    
    pthread_mutex_unlock(&d->mutex);
}

/* ============================================================
 * Function: main()
 * Entry point for baggage handling simulation
 * Creates loader, aircraft, and monitor threads
 * Return: 0 on success
 * ============================================================ */
int main(void)
{
    pthread_t loader_thread;
    pthread_t aircraft_thread;
    pthread_t monitor_thread;
    struct shared_data d;
    
    printf("\n========================================\n");
    printf("  AIRPORT BAGGAGE HANDLING SYSTEM\n");
    printf("========================================\n");
    printf("Belt Capacity: %d items\n", BELT_CAPACITY);
    printf("Loader speed: %d sec/item\n", LOADER_DELAY);
    printf("Aircraft speed: %d sec/item\n", AIRCRAFT_DELAY);
    printf("Total luggage: %d items\n", TOTAL_LUGGAGE);
    printf("Monitor interval: %d seconds\n", MONITOR_DELAY);
    printf("========================================\n\n");
    
    /* Initialize shared data */
    init_shared_data(&d);
    
    /* Create threads */
    if (pthread_create(&loader_thread, NULL, loader, &d) != 0)
    {
        fprintf(stderr, "Error creating loader thread\n");
        destroy_shared_data(&d);
        return (1);
    }
    
    if (pthread_create(&aircraft_thread, NULL, aircraft, &d) != 0)
    {
        fprintf(stderr, "Error creating aircraft thread\n");
        destroy_shared_data(&d);
        return (1);
    }
    
    if (pthread_create(&monitor_thread, NULL, monitor, &d) != 0)
    {
        fprintf(stderr, "Error creating monitor thread\n");
        destroy_shared_data(&d);
        return (1);
    }
    
    /* Wait for all threads to complete */
    pthread_join(loader_thread, NULL);
    pthread_join(aircraft_thread, NULL);
    pthread_join(monitor_thread, NULL);
    
    /* Print final statistics */
    printf("\n========================================\n");
    printf("        FINAL STATISTICS\n");
    printf("========================================\n");
    printf("Total luggage loaded:   %d\n", d.loaded);
    printf("Total luggage dispatched: %d\n", d.dispatched);
    printf("Final belt size:        %d/%d\n", d.count, BELT_CAPACITY);
    printf("========================================\n");
    
    /* Clean up */
    destroy_shared_data(&d);
    
    return (0);
}