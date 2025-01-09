#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define GROUP_SIZE 4

int main(int argc, char *argv[]) {
    int old_size, new_size;
    int old_rank, new_rank;
    int recv_rank;
    MPI_Comm custom_group;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &old_size); // Total number of processes.
    MPI_Comm_rank(MPI_COMM_WORLD, &old_rank); // The current process ID / Rank.

    // Split the MPI_COMM_WORLD into smaller groups of size GROUP_SIZE.
    int color = old_rank / GROUP_SIZE; // Determines group ID based on rank.
    MPI_Comm_split(MPI_COMM_WORLD, color, old_rank, &custom_group);

    // Get the rank and size within the new group.
    MPI_Comm_rank(custom_group, &new_rank);
    MPI_Comm_size(custom_group, &new_size);

    printf("Rank [%d] / size [%d] in MPI_COMM_WORLD and rank [%d] / size [%d] in custom group.\n",
           old_rank, old_size, new_rank, new_size);

    // Determine the ranks for sending and receiving within the group.
    int send_to = (new_rank + 1) % new_size; // Next process in the group.
    int recv_from = (new_rank - 1 + new_size) % new_size; // Previous process in the group.

    // Send the rank to the next process and receive from the previous process.
    MPI_Sendrecv(&new_rank, 1, MPI_INT, send_to, 0, &recv_rank, 1, MPI_INT, recv_from, 0, custom_group, MPI_STATUS_IGNORE);

    // Display the result.
    printf("Process [%d] from group [%d] received [%d].\n", new_rank, color, recv_rank);

    MPI_Comm_free(&custom_group); // Free the custom communicator.
    MPI_Finalize();
    return 0;
}
