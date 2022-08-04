##############################
### CFLAGS
##############################
CFLAGS = -Wall -g

##############################
### EFLAGS
##############################
VFLAGS = --leak-check=full --show-reachable=yes --track-origins=yes

##############################
### DIRECTORIES
##############################
INSTALL_DIR ?= install
SRC_DIR ?= src
TEST_DIR ?= tst

SRC_EX_DIR ?= src/exemple
##############################
### FILES
##############################
TST_OFF = 01-main 02-switch 03-equity 11-join 12-join-main 21-create-many 22-create-many-recursive 23-create-many-once 31-switch-many 32-switch-many-join 33-switch-many-cascade 51-fibonacci 71-preemption 61-mutex 62-mutex 80-debordements
TST_FILES = $(TST_OFF) 63-semaphore
EX_FILES = $(patsubst ${SRC_EX_DIR}/%.c, %,$(wildcard ${SRC_EX_DIR}/*.c))

##############################
### Règle all par défaut et règles dépot
##############################
all: libthread.so test

check: all
	LD_LIBRARY_PATH=. ./01-main
	LD_LIBRARY_PATH=. ./02-switch
	LD_LIBRARY_PATH=. ./03-equity
	LD_LIBRARY_PATH=. ./11-join
	LD_LIBRARY_PATH=. ./12-join-main
	LD_LIBRARY_PATH=. ./21-create-many 10000
	LD_LIBRARY_PATH=. ./22-create-many-recursive 10000
	LD_LIBRARY_PATH=. ./23-create-many-once 10
	LD_LIBRARY_PATH=. ./31-switch-many 8 7
	LD_LIBRARY_PATH=. ./32-switch-many-join 8 7
	LD_LIBRARY_PATH=. ./33-switch-many-cascade 8 7
	LD_LIBRARY_PATH=. ./51-fibonacci 8
	LD_LIBRARY_PATH=. ./71-preemption 5
	LD_LIBRARY_PATH=. ./51-fibonacci 3
	LD_LIBRARY_PATH=. ./61-mutex 20
	LD_LIBRARY_PATH=. ./62-mutex 20
	LD_LIBRARY_PATH=. ./63-semaphore 2
	LD_LIBRARY_PATH=. ./80-debordements

valgrind: all
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./01-main
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./02-switch
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./03-equity
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./11-join
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./12-join-main
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./21-create-many 10
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./22-create-many-recursive 10
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./23-create-many-once 10
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./31-switch-many 10 10
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./32-switch-many-join 10 10
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./33-switch-many-cascade 10 10
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./51-fibonacci 10
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./71-preemption 5
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./51-fibonacci 3
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./61-mutex 20
	LD_LIBRARY_PATH=. valgrind ${VFLAGS} ./62-mutex 20

install: tree
	mv ??-* install/bin/
	mv libthread.so install/lib/

graphs: all pthreads
	python3 src/graph/graph.py

##############################
### Création de l'arborescence
##############################
tree:
	mkdir -p install/bin install/lib

##############################
### Compilation et installation des exemples
##############################
exemple: ${EX_FILES}

%: src/exemple/%.c
	gcc $(CFLAGS) $< -o $@

##############################
### Compilation de notre lib personnelle
##############################
libthread.so: src/thread.c
	gcc $(CFLAGS) -shared -fPIC $< -o $@

##############################
### Compilation et installation des tests avec la lib officielle
##############################
pthreads: $(patsubst %,%-pthread,$(TST_OFF))

# utilisation DUSE_PTHREAD cf thread.h
%-pthread: tst/%.c
	gcc $(CFLAGS) -I./src -DUSE_PTHREAD -pthread $< -o $@

##############################
### Compilation et installation des tests avec la lib personnelle
##############################
test: ${TST_FILES}

# utiliser DUSE_PTHREAD cf thread.h
%: tst/%.c libthread.so
	gcc $(CFLAGS) -I./src -L. $< -lthread -o $@

##############################
### Mise au propre du répertoire
##############################
clean:
	rm -rf install build
	rm -f ??-*
	rm -f libthread.so
	rm -f ${EX_FILES}