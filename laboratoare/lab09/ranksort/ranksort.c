#include<mpi.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#define N 1000
#define MASTER 0

void compareVectors(int *a, int *b) {
    // DO NOT MODIFY
    int i;
    for (i = 0; i < N; i++) {
        if (a[i] != b[i]) {
            printf("Sorted incorrectly\n");
            return;
        }
    }
    printf("Sorted correctly\n");
}

void displayVector(int *v) {
    // DO NOT MODIFY
    int i;
    int displayWidth = 2 + log10(v[N - 1]);
    for (i = 0; i < N; i++) {
        printf("%*i", displayWidth, v[i]);
    }
    printf("\n");
}

int cmp(const void *a, const void *b) {
    // DO NOT MODIFY
    int A = *(int *)a;
    int B = *(int *)b;
    return A - B;
}

int main(int argc, char *argv[]) {
    int rank, i, j;
    int nProcesses;
    MPI_Init(&argc, &argv);
    int *v = (int *)malloc(sizeof(int) * N);
    int *vQSort = (int *)malloc(sizeof(int) * N);
    int *positions = (int *)malloc(sizeof(int) * N);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

    if (rank == MASTER) {
        // Generate random vector
        srand(42); // fixed seed for reproducibility
        for (i = 0; i < N; i++) {
            v[i] = rand() % 10000; // random values in range [0, 9999]
        }

        // Initialize positions
        for (i = 0; i < N; i++) {
            positions[i] = 0;
        }
    }

    // Send the vector to all processes
    MPI_Bcast(v, N, MPI_INT, MASTER, MPI_COMM_WORLD);

    // Calculate the range of indices for each process
    int start = (N / nProcesses) * rank;
    int end = (rank == nProcesses - 1) ? N : (N / nProcesses) * (rank + 1);

    // Each process computes positions for its part of the vector
    int *local_positions = (int *)malloc(sizeof(int) * (end - start));
    for (i = start; i < end; i++) {
        local_positions[i - start] = 0;
        for (j = 0; j < N; j++) {
            if (v[j] < v[i] || (v[j] == v[i] && j < i)) {
                local_positions[i - start]++;
            }
        }
    }

    // Gather all positions from all processes at the MASTER
    MPI_Gather(local_positions, end - start, MPI_INT, positions, end - start, MPI_INT, MASTER, MPI_COMM_WORLD);

    if (rank == MASTER) {
        // Display the original vector
        displayVector(v);

        // Make a copy to check it against qsort
        for (i = 0; i < N; i++) {
            vQSort[i] = v[i];
        }
        qsort(vQSort, N, sizeof(int), cmp);

        // Rearrange the vector based on the computed positions
        int *sorted_v = (int *)malloc(sizeof(int) * N);
        for (i = 0; i < N; i++) {
            sorted_v[positions[i]] = v[i];
        }

        // Copy the sorted vector back to v
        for (i = 0; i < N; i++) {
            v[i] = sorted_v[i];
        }

        // Display the sorted vector and compare it with qsort
        displayVector(v);
        compareVectors(v, vQSort);

        free(sorted_v);
    }

    free(v);
    free(vQSort);
    free(positions);
    free(local_positions);

    MPI_Finalize();
    return 0;
}
