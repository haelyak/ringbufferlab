/*
 * Producer/consumer program using a ring buffer (lab 5).
 *
 * Team members:
 *
 *      Kayleah Tsai <ktsai@cs.hmc.edu>
 *      Eric Chen <erchen@cs.hmc.edu>
 */

#include <pthread.h>
#include <stdio.h>
#include <time.h>

int     main(int argc, char *argv[]);
void *  producer(void * arg);
void *  consumer(void * arg);
void    thread_sleep(unsigned int ms);

#define BUFSLOTS        10

/*
 * Structure used to hold messages between producer and consumer.
 */
struct message {
    int value;          /* Value to be passed to consumer */
    int consumer_sleep; /* Time (in ms) for consumer to sleep */
    int line;           /* Line number in input file */
    int print_code;     /* Output code; see below */
    int quit;           /* Nonzero if consumer should exit ("value" ignored) */
};

/*
 * The ring buffer itself.
 */
static struct message   buffer[BUFSLOTS];

/*
 * Global variables
 */
int amount_produced = 0;
int amount_consumed = 0;

/*
 * Conditions and mutexes. 
 */
static pthread_cond_t   ccondition = PTHREAD_COND_INITIALIZER;
static pthread_cond_t   pcondition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t  mutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char * argv[])
{
    pthread_t   consumer_tid;


    /*
     * Make sure output appears right away.
     */
    setlinebuf(stdout);

    /*
     * Create a thread for the consumer.
     */
    if (pthread_create(&consumer_tid, NULL, consumer, NULL) != 0) {
        fprintf(stderr, "Couldn't create consumer thread\n");
        return 1;
    }

    /*
     * We will call the producer directly.  (Alternatively, we could
     * spawn a thread for the producer, but then we would have to join
     * it.)
     */
    producer(NULL);

    /*
     * The producer has terminated.  Clean up the consumer, which might
     * not have terminated yet.
     */
    if (pthread_join(consumer_tid, NULL) != 0) {
        fprintf(stderr, "Couldn't join with consumer thread\n");
        return 1;
    }
    return 0;
}


/*
* Producer adds stuff from file to buffer
*/
void * producer(void * arg)
{
    unsigned int        consumer_sleep; /* Space for reading in data */
    int                 line = 0;       /* Line number in input */
    int                 print_code;     /* Space for reading in data */
    unsigned int        producer_sleep; /* Space for reading in data */
    int                 value;          /* Space for reading in data */


    // Reads the file line by line until the end
    while (scanf("%d%u%u%d",
        &value, &producer_sleep, &consumer_sleep, &print_code)
      == 4) {

        line++;

        // Sleeps for a given amount of time
        thread_sleep(producer_sleep);

        // Locks the mutex to access the buffer
        if (pthread_mutex_lock(&mutex)!= 0){
            fprintf(stderr, "Mutex lock failed.");
            return NULL;
        }

        // Makes sure the producer doesn't lap the consumer
        while (amount_produced - amount_consumed >= 10) {
            if (pthread_cond_wait(&ccondition, &mutex)!=0){
                fprintf(stderr, "ccondition wait failed.");
                return NULL;
            }
        }

        // Puts the extracted data into the buffer
        buffer[amount_produced%10].value = value;
        buffer[amount_produced%10].consumer_sleep = consumer_sleep;
        buffer[amount_produced%10].line = line;
        buffer[amount_produced%10].print_code = print_code;
        buffer[amount_produced%10].quit = 0;

        amount_produced++;

        // Signals to consumer to let it know that something has been produced
        if (pthread_cond_signal(&pcondition)!=0){
            fprintf(stderr, "pcondition cond signal failed.");
            return NULL;
        }


        if (pthread_mutex_unlock(&mutex)!=0){
            fprintf(stderr, "Mutex unlock failed.");
            return NULL;
        }

        /*
         * After sending values to the consumer, print if the code is 1 or 3.
         */
        if (print_code == 1  ||  print_code == 3)
            printf("Produced %d from input line %d\n", value, line);
    }



    /*
     * Code to terminate the consumer (sends a quit).
     */
    if (pthread_mutex_lock(&mutex)!=0){
        fprintf(stderr, "Mutex lock failed.");
        return NULL;
    }

    // Makes sure the producer doesn't lap the consumer
    while (amount_produced - amount_consumed >= 10) {
        if (pthread_cond_wait(&ccondition, &mutex)!=0){
            fprintf(stderr, "ccondition wait failed.");
            return NULL;
        }
    }

    // Puts quit in the next message
    buffer[amount_produced%10].quit = 1;

    amount_produced++;

    if (pthread_cond_signal(&pcondition)!=0){
        fprintf(stderr, "pcondition cond signal failed.");
        return NULL;
    }
    if (pthread_mutex_unlock(&mutex)!=0){
        fprintf(stderr, "Mutex unlock failed.");
        return NULL;
    }

    return NULL;
}


/*
* Consumer extracts stuff from buffer; adds to sum
*/
void * consumer(void * arg)
{
    
    int sum = 0;
    int cvalue = 0;
    int csleep = 0;
    int cline = 0;
    int cprint_code = 0;
    int cquit = 0;

    // Keeps extracting from buffer
    while (1){

        // Locks mutex to access buffer
        if (pthread_mutex_lock(&mutex)!=0){
            fprintf(stderr, "Mutex lock failed.");
            return NULL;
        }

        // If consumer has caught up to producer, wait
        while (amount_consumed >= amount_produced) {
            if (pthread_cond_wait(&pcondition, &mutex)!=0){
                fprintf(stderr, "pcondition wait failed.");
                return NULL;
            }
        }

        // Checks if the consumer should quit
        cquit = buffer[amount_consumed%10].quit;
        
        if (cquit != 0) {
            printf("Final sum is %d\n", sum);
            return NULL;
        }

        // Extracting message from buffer
        cvalue = buffer[amount_consumed%10].value;
        csleep = buffer[amount_consumed%10].consumer_sleep;
        cline = buffer[amount_consumed%10].line;
        cprint_code = buffer[amount_consumed%10].print_code;

        amount_consumed++;

        // Signals producer that it finished extracting
        if (pthread_cond_signal(&ccondition)!=0){
            fprintf(stderr, "ccondition cond signal failed.");
            return NULL;
        }

        if (pthread_mutex_unlock(&mutex) != 0){
            fprintf(stderr, "Mutex unlock failed.");
            return NULL;
        }

        // Sleeps for given amount of time
        thread_sleep(csleep);
        
        sum += cvalue;

        // Generate status print message (when 2 and 3)
        if (cprint_code == 2  ||  cprint_code == 3){
            printf("Consumed %d from input line %d; sum = %d\n", cvalue, cline, sum);
        }
    }
    return NULL;
}

/*
* Wrapper for nanosleep function
*/
void thread_sleep(unsigned int ms)
{
    struct timespec     sleep_time;

    if (ms == 0)
        return;

    sleep_time.tv_sec = ms/1000;
    sleep_time.tv_nsec = ms%1000 *1000000;
    if (nanosleep(&sleep_time, NULL) != 0) {
        fprintf(stderr, "There was an error with nanosleep.\n");
    }
        
}
