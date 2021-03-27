//Author: Andrae Muys (1997)
//Revised: Dimitris Spanos (2021)
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define QUEUESIZE 10
#define LOOP 10000
#define p 1// number of producer threads
#define c 4 // number of consumer threads

int consumed = 0;
double average = 0;


void *example_work(){
    double k;
    for(double i=0; i<10; i++)
        k = sin(i);
    return (NULL);
}

typedef struct {
    void * (*work)(void *);
    void * arg;
    struct timespec start;
}workFunction;

double time_spent(struct timespec start,struct timespec end);
void *producer (void *args);
void *consumer (void *args);

typedef struct {
  workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;


queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);

int main ()
{

  int status;
  queue *fifo;

  pthread_t *prod_threads;
  prod_threads = (pthread_t *) malloc(p* sizeof(pthread_t));

  pthread_t *con_threads;
  con_threads = (pthread_t *) malloc(c* sizeof(pthread_t));

  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  /*
   ---------------------------------
          Creation of threads
   ---------------------------------
  */
  for(long t=0; t<p; t++){
    status = pthread_create(&prod_threads[t],NULL,producer,fifo);
    if (status){
      printf("ERROR; return code from pthread_create() is %d\n", status);
      exit(-1);
    }
  }

  for(long t=0; t<c; t++){
    status = pthread_create(&con_threads[t],NULL,consumer,fifo);
    if (status){
      printf("ERROR; return code from pthread_create() is %d\n", status);
      exit(-1);
    }
  }

  printf("\n ** Prod-Cons **\nWorkFunctions: %d  Producers: %d  Consumers: %d\n", LOOP*p, p, c);
  for (int i=0; i<p; i++)
    pthread_join(prod_threads[i], NULL);
  for (int i=0; i<c; i++)
    pthread_join(con_threads[i], NULL);

  queueDelete (fifo);
  average /= (double)(LOOP*(double)p);
  printf("Average Waiting Time: %f microsec\n", average * (double)1000000);

  pthread_exit(NULL);
  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  workFunction w;
  w.work = example_work;
  int i;
  fifo = (queue *)q;

  for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full)
      pthread_cond_wait (fifo->notFull, fifo->mut);

    clock_gettime(CLOCK_MONOTONIC, &w.start);
    queueAdd (fifo, w);
    pthread_cond_signal (fifo->notEmpty);
    pthread_mutex_unlock (fifo->mut);

  }
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  workFunction d;
  fifo = (queue *)q;

  while(1) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      // if all the workFunctions are consumed, unlock and stop this consumer
      // and signal to the rest that they too should stop.
      if (consumed == p * LOOP){
        pthread_cond_signal (fifo->notEmpty);
        pthread_mutex_unlock (fifo->mut);
        return (NULL);
      }
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }

    struct timespec end;
    clock_gettime( CLOCK_MONOTONIC, &end);
    queueDel (fifo, &d);
    consumed += 1;
    average += time_spent(d.start, end);
    d.work;

    pthread_cond_signal (fifo->notFull);
    pthread_mutex_unlock (fifo->mut);

  }

  return (NULL);
}

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);

  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, workFunction in)
{
  q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction *out)
{
  *out = q->buf[q->head];

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}

double time_spent(struct timespec start,struct timespec end)
{
        struct timespec temp;
        if ((end.tv_nsec - start.tv_nsec) < 0)
        {
                temp.tv_sec = end.tv_sec - start.tv_sec - 1;
                temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
        }
        else
        {
                temp.tv_sec = end.tv_sec - start.tv_sec;
                temp.tv_nsec = end.tv_nsec - start.tv_nsec;
        }
        return (double)temp.tv_sec +(double)((double)temp.tv_nsec/(double)1000000000);

}
