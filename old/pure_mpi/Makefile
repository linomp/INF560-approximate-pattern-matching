SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=gcc
MPI_CC=mpicc
CFLAGS=-O3 -I$(HEADER_DIR) -Wall
LDFLAGS=

SRC= apm_patterns_over_ranks.c

OBJ_MPI_1= $(OBJ_DIR)/apm_patterns_over_ranks.o

all: $(OBJ_DIR) apm_patterns_over_ranks

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(MPI_CC) $(CFLAGS) -c -o $@ $^

apm_patterns_over_ranks:$(OBJ_MPI_1)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f apm apm_patterns_over_ranks $(OBJ_MPI_1) ; rmdir $(OBJ_DIR)
