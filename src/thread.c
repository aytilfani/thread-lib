#include "thread.h"
#include <assert.h>
#include <valgrind/valgrind.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

//define the USE_PREEMPTION macro to enable preemption
//#define USE_PREEMPTION

thread_t main_thread = NULL;
int last_id = 0; //the numbers of id

#if defined(USE_PREEMPTION)
int time_slice = 200000; // the time slice before automatic yield (in nanoseconds)
int lock_auto_yield = 0; // if 1, lock auto yield by SIGALARM
#endif

SIMPLEQ_HEAD(threadqueue, thread_t) QUEUE;

//fonction pour debug
void show_queue(struct thread_t *np){
  printf("=== SHOW QUEUE ===\n");
  SIMPLEQ_FOREACH(np, &QUEUE, threads) {
        printf("%d\n", np->id);
    }
  printf("=== END QUEUE ===\n");
}

#if defined(USE_PREEMPTION)
// set a timer raising a SIGALARM signal
void set_timer()
{
  // printf("set_timer reset\n");
  struct itimerval itv;
  itv.it_value.tv_sec = 0;
  itv.it_value.tv_usec = time_slice;
  itv.it_interval.tv_sec = 0;
  itv.it_interval.tv_usec = 0;
  if(setitimer(ITIMER_REAL, &itv, NULL) == -1){
    perror("setitimer");
    exit(1);
  }
}

// Catch SIGALARM signal
void sig_handler(int signum){
  if (signum != SIGALRM)
    return;

  if (lock_auto_yield){
    set_timer();
  }
  else{
    printf("=> auto yield <=\n");
    thread_yield();
  }
}
#endif

void handler(int sig) {
  if(sig == SIGSEGV)
    printf("Alternative Stack used after segfault\n");
  SIMPLEQ_FIRST(&QUEUE)->segfault = 1;
  thread_exit(NULL);
}

//fonction permettant d'assurer l'appel au thread_exit
void f(void * n){
  #if defined(USE_PREEMPTION)
  // reset timer
  set_timer();

  lock_auto_yield--;
  // printf("f: lock_auto_yield: %d\n", lock_auto_yield);
  #endif

  thread_t t = (thread_t) n;
  thread_exit(t->func(t->funcarg));
}

char altstack[SIGSTKSZ];

//fonction s'executant au début du programme
//pour initialiser le thread principal
void thread_init(void){
  stack_t s  = {
    .ss_sp = altstack,
    .ss_size = SIGSTKSZ,
    .ss_flags = 0,
  };
  struct sigaction sa = {
    .sa_handler = handler,
    .sa_flags = SA_ONSTACK,
  };
  int ret = sigaltstack(&s, 0);
  if(ret != 0)
    printf("sigalstack error\n");
  sigemptyset(&sa.sa_mask);
  ret = sigaction(SIGSEGV, &sa, NULL);
  if(ret != 0) 
    printf("sigaction error\n");
  main_thread = malloc(sizeof(struct thread_t));
  main_thread->id = last_id;
  main_thread->finished = 0;
  main_thread->segfault = 0;
  main_thread->wait_thread = NULL;
  last_id+=1;

  SIMPLEQ_INIT(&QUEUE);
  SIMPLEQ_INSERT_HEAD(&QUEUE, main_thread, threads);

  #if defined(USE_PREEMPTION)
  // Set up the signal handler
  signal(SIGALRM, sig_handler);
  #endif
}

//fonction s'executant à la fin du programme
//pour vider la mémoire
void thread_end(void){
  free(SIMPLEQ_FIRST(&QUEUE));
  stack_t s = {.ss_flags = SS_DISABLE};
  stack_t old;
  sigaltstack(&s,&old);
}


void __attribute__((constructor)) thread_init();
void __attribute__((destructor)) thread_end();

thread_t thread_self(void){
  return SIMPLEQ_FIRST(&QUEUE);
}

/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */
int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg){
  thread_t thread = malloc(sizeof(struct thread_t));
  thread->id = last_id;
  last_id+=1;
  thread->finished = 0;
  thread->segfault = 0;
  thread->wait_thread = NULL;
  getcontext(&(thread->context));
  thread->context.uc_stack.ss_size = 64*1024;
  int pagesize = sysconf(_SC_PAGE_SIZE);
  posix_memalign(&thread->stack, pagesize, pagesize + thread->context.uc_stack.ss_size);
  thread->context.uc_stack.ss_sp = thread->stack + pagesize;
  mprotect(&thread->stack, pagesize, PROT_NONE); 
  thread->valgrind_stackid = VALGRIND_STACK_REGISTER(thread->context.uc_stack.ss_sp, thread->context.uc_stack.ss_sp + thread->context.uc_stack.ss_size);
  thread->context.uc_link = NULL;
  thread->func = (void *) func;
  thread->funcarg = funcarg;
  makecontext(&(thread->context), (void (*)) f, 1, thread);

  #if defined(USE_PREEMPTION)
  // lock auto yield
  lock_auto_yield++;
  // printf("thread_create: lock_auto_yield: %d\n", lock_auto_yield);

  // reset timer
  set_timer();

  #endif


  thread_t temp = SIMPLEQ_FIRST(&QUEUE);
  SIMPLEQ_INSERT_HEAD(&QUEUE, thread, threads);

  swapcontext(&temp->context, &thread->context);

  #if defined(USE_PREEMPTION)
  // unlock auto yield
  lock_auto_yield--;
  // printf("thread_create: lock_auto_yield: %d\n", lock_auto_yield);
  #endif

  //printf("Sortir\n");

  *newthread = thread;

  return 0;
}


/* passer la main à un autre thread.
 */
int thread_yield(void) {
  #if defined(USE_PREEMPTION)
  // lock auto yield
  lock_auto_yield++;
  // printf("thread_yield: lock_auto_yield: %d\n", lock_auto_yield);
  #endif

  assert(!SIMPLEQ_EMPTY(&QUEUE));
  if (SIMPLEQ_NEXT(SIMPLEQ_FIRST(&QUEUE), threads) == NULL){
    #if defined(USE_PREEMPTION)
    lock_auto_yield--;
    // printf("thread_yield: lock_auto_yield: %d\n", lock_auto_yield);
    #endif

    return 0;
  }
  thread_t temp = SIMPLEQ_FIRST(&QUEUE);
  SIMPLEQ_REMOVE_HEAD(&QUEUE, threads);
  SIMPLEQ_INSERT_TAIL(&QUEUE, temp, threads);

  #if defined(USE_PREEMPTION)
  // reset the timer
  set_timer();
  #endif

  swapcontext(&temp->context, &SIMPLEQ_FIRST(&QUEUE)->context);

  #if defined(USE_PREEMPTION)
  // unlock the lock
  lock_auto_yield--;
  // printf("thread_yield: lock_auto_yield: %d\n", lock_auto_yield);
  #endif

  return 0;
}

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t thread, void **retval){
  #if defined(USE_PREEMPTION)
  // lock auto yield to allow the thread join
  lock_auto_yield++;
  // printf("thread_join: lock_auto_yield: %d\n", lock_auto_yield);
  #endif

  //check if the thread is finished
  if (thread->finished == 0){
    // printf("thread_join: thread not finished\n");
    //ajoute le thread en cours d'execution a la sleepqueue
    thread->wait_thread = thread_self();
    assert(!SIMPLEQ_EMPTY(&QUEUE));
    thread_t temp = SIMPLEQ_FIRST(&QUEUE);
    SIMPLEQ_REMOVE_HEAD(&QUEUE, threads);
    swapcontext(&temp->context, &SIMPLEQ_FIRST(&QUEUE)->context);
  }
  if (retval != NULL)
    *retval = thread->retval;
  
  #if defined(USE_PREEMPTION)
  // reset the timer
  set_timer();

  // unlock the lock
  lock_auto_yield--;
  // printf("thread_join: lock_auto_yield: %d\n", lock_auto_yield);
  #endif

  if (thread->segfault == 1) {
    VALGRIND_STACK_DEREGISTER(thread->valgrind_stackid);
    if (thread->id != 0)
      free(thread->stack);
    free(thread);
    return -1;
  }
  VALGRIND_STACK_DEREGISTER(thread->valgrind_stackid);
  if (thread->id != 0)
    free(thread->stack);
  free(thread);

  return 0;
}

/* terminer le thread courant en renvoyant la valeur de retour retval.
 * cette fonction ne retourne jamais.
 *
 * L'attribut noreturn aide le compilateur à optimiser le code de
 * l'application (élimination de code mort). Attention à ne pas mettre
 * cet attribut dans votre interface tant que votre thread_exit()
 * n'est pas correctement implémenté (il ne doit jamais retourner).
 */
void thread_exit(void *retval){
  #if defined(USE_PREEMPTION)
  lock_auto_yield++;
  // printf("thread_exit: lock_auto_yield: %d\n", lock_auto_yield);
  #endif

  if (SIMPLEQ_NEXT(SIMPLEQ_FIRST(&QUEUE), threads) == NULL)
  {
    thread_t current_thread = SIMPLEQ_FIRST(&QUEUE);
    //si un seul thread dans le runqueue et
    //aucun thread dans la sleepqueue on quitte le programme
    if (current_thread->wait_thread == NULL)
      exit(0);
  }

  thread_t temp = SIMPLEQ_FIRST(&QUEUE);
  temp->finished = 1;
  temp->retval = retval;

  //insertion du thread de la sleepqueue juste apres le 
  //thread en cours d'execution
  if (temp->wait_thread != NULL)
  {
    SIMPLEQ_INSERT_AFTER(&QUEUE, temp, temp->wait_thread, threads);
    temp->wait_thread = NULL;
  }
  SIMPLEQ_REMOVE_HEAD(&QUEUE, threads);

  setcontext(&SIMPLEQ_FIRST(&QUEUE)->context);
  abort();

  #if defined(USE_PREEMPTION)
  // reset the timer
  set_timer();

  // lock_auto_yield--;
  #endif
}

int thread_mutex_init(thread_mutex_t* mutex){
  mutex->lock = 0;  
  SIMPLEQ_INIT(&mutex->QUEUEMUTEX);
  return 0;
}

int thread_mutex_destroy(thread_mutex_t* mutex){
  return 0;
}

int thread_mutex_lock(thread_mutex_t* mutex){
  while (mutex->lock == 1){
    if (SIMPLEQ_NEXT(SIMPLEQ_FIRST(&QUEUE), threads) != NULL) {
      thread_t temp = SIMPLEQ_FIRST(&QUEUE);
      SIMPLEQ_REMOVE_HEAD(&QUEUE, threads);
      SIMPLEQ_INSERT_TAIL(&mutex->QUEUEMUTEX, temp, threads);
      swapcontext(&temp->context, &SIMPLEQ_FIRST(&QUEUE)->context);  
    }
  }
  mutex->lock = 1;
  return 0;
}

int thread_mutex_unlock(thread_mutex_t* mutex){
  if (SIMPLEQ_EMPTY(&mutex->QUEUEMUTEX) != 1) {
    thread_t temp = SIMPLEQ_FIRST(&mutex->QUEUEMUTEX);
    SIMPLEQ_REMOVE_HEAD(&mutex->QUEUEMUTEX, threads);
    SIMPLEQ_INSERT_AFTER(&QUEUE, SIMPLEQ_FIRST(&QUEUE), temp, threads);
  }
  mutex->lock = 0;
  return 0;
}

int sem_init(sem_t *sem, int pshared, unsigned int value){
  sem->val = value;
  SIMPLEQ_INIT(&sem->QUEUESEM);
  return 0;
}

int sem_destroy(sem_t *sem){
  return 0;
}

int sem_wait(sem_t *sem){
  while (sem->val != 0){
    if (SIMPLEQ_NEXT(SIMPLEQ_FIRST(&QUEUE), threads) != NULL) {
      thread_t temp = SIMPLEQ_FIRST(&QUEUE);
      SIMPLEQ_REMOVE_HEAD(&QUEUE, threads);
      SIMPLEQ_INSERT_TAIL(&sem->QUEUESEM, temp, threads);
      swapcontext(&temp->context, &SIMPLEQ_FIRST(&QUEUE)->context);  
    }
  }
  sem->val -= 1;
  return 0;
}

int sem_post(sem_t *sem){
  if (SIMPLEQ_EMPTY(&sem->QUEUESEM) != 1) {
    thread_t temp = SIMPLEQ_FIRST(&sem->QUEUESEM);
    SIMPLEQ_REMOVE_HEAD(&sem->QUEUESEM, threads);
    SIMPLEQ_INSERT_AFTER(&QUEUE, SIMPLEQ_FIRST(&QUEUE), temp, threads);
  }
  sem->val = sem->val + 1;
  return 0;
}