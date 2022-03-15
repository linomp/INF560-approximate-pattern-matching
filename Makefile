SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=gcc
SEQ_FLAGS=-O3 -I$(HEADER_DIR) -Wall

MPI_CC=mpicc
NV_CC=nvcc
CFLAGS=-O3 -I$(HEADER_DIR) -Wall -fopenmp -DAPM_INFO -DDEBUG_APPROACH_CHOSEN -DUSE_GPU -DDEBUG_CUDA
LDFLAGS=-lm -lcudart -L/usr/local/cuda/lib64

SRC= main.c patterns_over_ranks.c database_over_ranks.c utils.c sequential.c

OBJ= $(OBJ_DIR)/patterns_over_ranks.o $(OBJ_DIR)/database_over_ranks.o $(OBJ_DIR)/main.o $(OBJ_DIR)/utils.o

all: $(OBJ_DIR) cuda_utils apm_parallel apm_sequential

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(MPI_CC) $(CFLAGS) -c -o $@ $^

utils:$(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

apm_sequential:$(OBJ_DIR)/utils.o $(OBJ_DIR)/sequential.o
	$(CC) $(SEQ_FLAGS) $(LDFLAGS) -o $@ $^

database_over_ranks:$(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

patterns_over_ranks:$(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

cuda_utils:
	$(NV_CC) -c $(SRC_DIR)/cuda_utils.cu -o $(OBJ_DIR)/cuda_utils.o

apm_parallel: $(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(OBJ_DIR)/cuda_utils.o

clean:
	rm -f apm_parallel apm_sequential $(OBJ) ; rmdir $(OBJ_DIR)
