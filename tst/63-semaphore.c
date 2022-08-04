#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include "thread.h"

/* test de faire une somme avec plein de thread sur un compteur partagé
 *
 * valgrind doit etre content.
 * Les résultats doivent etre égals au nombre de threads * 1000.
 * La durée du programme doit etre proportionnelle au nombre de threads donnés en argument.
 *
 * support nécessaire:
 * - thread_create()
 * - retour sans thread_exit()
 * - thread_join() sans récupération de la valeur de retour
 * - thread_mutex_init()
 * - thread_mutex_destroy()
 * - thread_mutex_lock()
 * - thread_mutex_unloc()
 */

#define NB_SEMAPHORE 10
#define NBR_TESTS 10
sem_t semaphore;

void* func1(void* args){
    printf("Je suis le premier thread\n");
    for (int i = 0; i < NBR_TESTS; i++){
        printf("Avant le YIELD %d\n", i);
        thread_yield();
        thread_yield();
        printf("Avant le post %d\n", i);
        sem_post(&semaphore);
    }
    printf("Je vais quitter le premier thread\n");
    return 0;
}

void* func2(void* args){
    printf("Je suis le second thread\n");
    for (int i = 0; i < NBR_TESTS; i++){
        printf("Avant le wait %d\n", i);
        sem_wait(&semaphore);
    }
    printf("Je vais quitter le second thread\n");
    return 0;
}

int main(int argc, char *argv[])
{
    thread_t th[2];

    if (argc < 2) {
        printf("argument manquant: nombre de threads\n");
        return -1;
    }

    sem_init(&semaphore, 0, 0);

    /* on cree tous les threads */
    thread_create(&th[0], func1, NULL);
    thread_create(&th[1], func2, NULL);

    void* ret = NULL;
    thread_join(th[0], ret);
    thread_join(th[1], ret);
    sem_destroy(&semaphore);

    return 0;
}
