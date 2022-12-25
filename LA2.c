#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpi.h>
#include <unistd.h>
#define NUM_FLOORS 3
#define SPACE_PER_FLOOR 50

int total_slots;             // total number of parking spaces in the lot
int floor_slots[NUM_FLOORS]; // number of available spaces on each floor
pthread_mutex_t lock;        // mutex lock for accessing shared variables

// Function for updating the display with current number of free slots
void update_display(int floor, int slots)
{
    pthread_mutex_lock(&lock);  // lock the mutex
    floor_slots[floor] = slots; // update number of free slots on the floor
    total_slots = 0;            // reset total number of free slots
    for (int i = 0; i < NUM_FLOORS; i++)
        total_slots += floor_slots[i]; // sum number of free slots on each floor
    printf("Updated display: Floor %d has %d free slots. Total %d free slots in the lot.\n", floor, slots, total_slots);
    pthread_mutex_unlock(&lock); // unlock the mutex
}

void *display_thread(void *arg)
{
    int *floor_ptr = (int *)arg; // get floor number from argument and cast to integer pointer
    int floor = *floor_ptr;      // dereference pointer to get floor number
    int slots;                   // number of free slots on the floor
    for (int i = 0; i < 3; i++)  // run for 3 cycles of 5 minutes
    {
        slots = rand() % SPACE_PER_FLOOR; // generate random number of free slots
        update_display(floor, slots);     // update the display with current number of free slots
        sleep(5);                         // wait for 5 minutes
    }
    free(floor_ptr); // free the memory allocated for the floor number
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int rank, size;
    pthread_t threads[NUM_FLOORS];   // array of threads
    pthread_mutex_init(&lock, NULL); // initialize the mutex lock
    MPI_Init(&argc, &argv);          // initialize MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Create threads for each floor
    for (int i = 0; i < NUM_FLOORS; i++)
    {
        int *floor_ptr = malloc(sizeof(int));                         // allocate memory for floor number
        *floor_ptr = i + 1;                                           // store floor number in allocated memory
        pthread_create(&threads[i], NULL, display_thread, floor_ptr); // pass pointer to floor number as argument
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_FLOORS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Send updated parking lot information to other MPI processes
    int updated_info[NUM_FLOORS + 1]; // array to store updated parking lot information
    for (int i = 0; i < NUM_FLOORS; i++)
        updated_info[i] = floor_slots[i];   // store number of free slots on each floor
    updated_info[NUM_FLOORS] = total_slots; // store total number of free slots in the lot
    for (int i = 0; i < size; i++)
    {
        if (i != rank)
        {
            MPI_Send(updated_info, NUM_FLOORS + 1, MPI_INT, i, 0, MPI_COMM_WORLD); // send updated information to other processes
        }
    }

    // Receive updated parking lot information from other MPI processes
    int recv_info[NUM_FLOORS + 1]; // array to store received parking lot information
    for (int i = 0; i < size; i++)
    {
        if (i != rank)
        {
            MPI_Recv(recv_info, NUM_FLOORS + 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // receive updated information from other processes
            // Update local parking lot information with received information
            for (int j = 0; j < NUM_FLOORS; j++)
                floor_slots[j] = recv_info[j];
            total_slots = recv_info[NUM_FLOORS];
        }
    }

    pthread_mutex_destroy(&lock); // destroy the mutex
    MPI_Finalize();               // finalize MPI
    return 0;
}
