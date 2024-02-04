#include <stdio.h>
#include <mpi.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define ARRAY_SIZE 64
#define PROCESSES 8
#define BLOCK_SIZE ARRAY_SIZE/PROCESSES

void initialize_array_block(int arr[BLOCK_SIZE], int rank);
void compare_split(int local_arr[BLOCK_SIZE], int recv_local_arr[BLOCK_SIZE], int merged_arr[2 * BLOCK_SIZE], int keep_small);
void write_file(int arr[ARRAY_SIZE]);
int incOrder(const void *e1, const void *e2);
// void write_file_test(int matrix[PROCESSES_IN_1D*PROCESSES_IN_1D][BLOCK_SIZE][BLOCK_SIZE]);
void write_file_rank(int arr[BLOCK_SIZE], int rank);
void write_file_rank_mod(int arr1[BLOCK_SIZE], int arr2[BLOCK_SIZE], int rank, int i, int j);

int local_arr[BLOCK_SIZE], result_arr[ARRAY_SIZE];

int main(int argc, char* argv[]) {
  struct timespec begin, end;
  int rank, procs;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &procs);

  int recv_local_arr[BLOCK_SIZE], merged_arr[2 * BLOCK_SIZE];
  if (rank == 0) {
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
  }
  // each process is initiallized with part of globally reverse sorted array e.g. p0: 64,63,62,61,60,59,58,57 etc
  initialize_array_block(local_arr, rank);

  // local sort
  qsort(local_arr, BLOCK_SIZE, sizeof(int), incOrder);
  // write_file_rank(local_arr, rank);

  int dimensions = (int) log2(PROCESSES);
  int i, j, tag = 0;
  // loop is same as bitonic sort on hypercube,
  // just comp_exchange is replaced by compare_split operation as mentioned in the book.
  for(i = 0; i < dimensions; i++) {
    for(j = i; j >= 0; j--) {
      int mask = 1 << j; 
      int dest = (rank ^ mask);

      MPI_Sendrecv(&local_arr[0], BLOCK_SIZE, MPI_INT, dest, tag,
        &recv_local_arr[0], BLOCK_SIZE, MPI_INT, dest, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      // write_file_rank_mod(local_arr, recv_local_arr, rank, i, j);

      if (((rank >> (i + 1)) & 1) != ((rank >> (j)) & 1)) {
        compare_split(local_arr, recv_local_arr, merged_arr, 0);
      } else {
        compare_split(local_arr, recv_local_arr, merged_arr, 1);
      }
      // write_file_rank_mod(local_arr, recv_local_arr, rank, i, j);

      tag++;
    }
  }

  // write_file_rank(local_arr, rank);

  MPI_Gather(&local_arr[0], BLOCK_SIZE, MPI_INT,
    result_arr, BLOCK_SIZE, MPI_INT, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    printf ("Total time = %f seconds\n",
      (end.tv_nsec - begin.tv_nsec) / 1000000000.0 +
      (end.tv_sec  - begin.tv_sec));
    write_file(result_arr);
  }

  MPI_Finalize();
}

void initialize_array_block(int arr[BLOCK_SIZE], int rank) {
  int i, j;
  for (i = 0, j = BLOCK_SIZE; i < BLOCK_SIZE, j >= 1; i++, j--) {
    arr[i] = ((PROCESSES - (rank + 1)) * BLOCK_SIZE) + j;
  }
}

void compare_split(int arr1[BLOCK_SIZE], int arr2[BLOCK_SIZE], int merged_arr[2 * BLOCK_SIZE], int keep_small) {
  int i = 0, j = 0, k = 0;
  while (i < BLOCK_SIZE && j < BLOCK_SIZE)
  {
    if (arr1[i] < arr2[j]) {
      merged_arr[k++] = arr1[i++];
    }
    else {
      merged_arr[k++] = arr2[j++];
    }
  }

  while (i < BLOCK_SIZE) {
    merged_arr[k++] = arr1[i++];
  }

  while (j < BLOCK_SIZE) {
    merged_arr[k++] = arr2[j++];
  }

  i = 0;
  if (keep_small == 1) {
    while (i < BLOCK_SIZE) {
      arr1[i] = merged_arr[i];
      i++;
    }
  } else {
    while (i < BLOCK_SIZE) {
      arr1[i] = merged_arr[i + BLOCK_SIZE];
      i++;
    }
  }
}

void write_file(int arr[ARRAY_SIZE]) {
  FILE *fp1;

  fp1 = fopen("./result_mpi.txt", "w+");

  int i, j;
  for(i = 0; i < ARRAY_SIZE; i++) {
    fprintf(fp1, "%d ", arr[i]);
  }
  fprintf(fp1, "\n");

  fclose(fp1);
}

int incOrder(const void *e1, const void *e2) {
  return (*((int *)e1) - *((int *)e2));
}

// util functions
void write_file_rank(int arr[BLOCK_SIZE], int rank) {
  FILE *fp1;

  char buf[256];
  snprintf(buf, sizeof(buf), "./rank_%d.txt", rank);
  fp1 = fopen(buf, "w+");

  int i, j;

  fprintf(fp1, "-------------------------------------\n");
  fprintf(fp1, "rank=%d\n", rank);
  for (i = 0; i < BLOCK_SIZE; i++) {
    fprintf(fp1, "%d\n", arr[i]);
  }
  fprintf(fp1, "-------------------------------------\n");

  fclose(fp1);
}

void write_file_rank_mod(int arr1[BLOCK_SIZE], int arr2[BLOCK_SIZE], int rank, int p, int q) {
  FILE *fp1;

  char buf[256];
  snprintf(buf, sizeof(buf), "./rank_%d.txt", rank);
  fp1 = fopen(buf, "a");

  int i, j;

  fprintf(fp1, "-------------------------------------\n");
  fprintf(fp1, "rank=%d i=%d j=%d\n", rank, p, q);
  for (i = 0; i < BLOCK_SIZE; i++) {
    fprintf(fp1, "%d %d\n", arr1[i], arr2[i]);
  }
  fprintf(fp1, "-------------------------------------\n");

  fclose(fp1);
}
