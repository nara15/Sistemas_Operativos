//https://users.cs.cf.ac.uk/Dave.Marshall/C/node32.html
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>

#include "threadpool.h"


#if !defined(DISABLE_PRINT)
#define err(str) fprintf(stderr, str)
#else
#define err(str)
#endif


static volatile int threads_keepalive;
static volatile int threads_on_hold;



/* ========================== ESTRUCTURAS ============================ */


// El semáforo binario para controlar la sincronización
typedef struct B_semaphore 
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int v;
} B_semaphore;


//	La estructura para controlar el trabajo de los hilos
typedef struct Job
{
	struct Job*  previous;					//	Puntero al trabajo anterior
	void*  arg;                         	//	Argumentos de la función
	void   (*function)(void* arg);			//	Puntero a la función       
} Job;


//	La fila de trabajos
typedef struct Job_Queue
{
	int   len;                          	//	Cantidad de trabajos en la pila
	Job  *front_q;                      	//	Puntero al frente de la pila
	Job  *rear_q;                       	//	Puntero al final de la pila
	B_semaphore *has_jobs;              	//	Semáforo binario
	pthread_mutex_t rwmutex;            	//	Acceso r/w a la fila
} Job_Queue;


//	Los hilos del pool
typedef struct Thread
{
	int       id;                       	//	Identificador del hilo
	pthread_t pthread;                  	//	Puntero al hilo actual
	struct ThreadPool* thread_pool;         //	Acceso al pool
} Thread;


//	Piscina de hilos
typedef struct ThreadPool
{
	Thread**   threads;                 	//	Puntero a los hilos
	volatile int num_threads_working;    	//	Cantidad de hilos actualmente trabajando
	volatile int num_threads_alive;     	//	Cantidad de hilos aun vivos
	pthread_mutex_t  thread_count_lock;     //	Semáforo para modificación
	pthread_cond_t  threads_all_idle;   	//	Señal de espera
	Job_Queue  Job_Queue;               	//	Fila de trabajos
} ThreadPool;





/* ========================== PROTOTIPOS ============================ */


static int  init_Thread(ThreadPool* thread_pool, struct Thread** thread_p, int id);
static void* do_Thread(struct Thread* thread_p);
static void  hold_Thread(int sig_id);
static void  destroy_Thread(struct Thread* thread_p);

static int   init_Job_Queue(Job_Queue* jobqueue_p);
static void  clear_Job_Queue(Job_Queue* jobqueue_p);
static void  push_Job_Queue(Job_Queue* jobqueue_p, struct Job* newjob_p);
static struct Job* pull_Job_Queue(Job_Queue* jobqueue_p);
static void  destroy_Job_Queue(Job_Queue* jobqueue_p);

static void  init_semaphore(struct B_semaphore *p_semaphore, int value);
static void  reset_semaphore(struct B_semaphore *p_semaphore);
static void  notify_by_semaphore(struct B_semaphore *p_semaphore);
static void  notify_ALL_by_semaphore(struct B_semaphore *p_semaphore);
static void  wait_semaphore(struct B_semaphore *p_semaphore);





/* ========================== THREADPOOL ============================ */


//	Inicializar el pool de hilos
struct ThreadPool* init_ThreadPool(int num_threads)
{

	threads_on_hold   = 0;
	threads_keepalive = 1;

	if (num_threads < 0){
		num_threads = 0;
	}

	//	Realizar un nuevo pool de hilos
	ThreadPool* thread_pool;
	thread_pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
	if (thread_pool == NULL)
	{
		err("init_ThreadPool(): NO se puede obtener memoria para el pool\n");
		return NULL;
	}
	thread_pool->num_threads_alive   = 0;
	thread_pool->num_threads_working = 0;

	//	Inicalizar la pila de trabajo
	if (init_Job_Queue(&thread_pool->Job_Queue) == -1)
	{
		err("init_ThreadPool(): NO se puede obtener memoria para la pila de trabajo\n");
		free(thread_pool);
		return NULL;
	}

	/* Hacer los hilos del pool */
	thread_pool->threads = (struct Thread**)malloc(num_threads * sizeof(struct Thread *));
	if (thread_pool->threads == NULL)
	{
		err("init_ThreadPool(): NO se puede obtener memoria para los hilos de ejecución\n");
		destroy_Job_Queue(&thread_pool->Job_Queue);
		free(thread_pool);
		return NULL;
	}

	pthread_mutex_init(&(thread_pool->thread_count_lock), NULL);
	pthread_cond_init(&thread_pool->threads_all_idle, NULL);

	//	Se inicializan los hilos
	int n;
	for (n=0; n<num_threads; n++)
	{
		init_Thread(thread_pool, &thread_pool->threads[n], n);
	}

	//	Esperar que los hilos se inicalicen
	while (thread_pool->num_threads_alive != num_threads) {}

	return thread_pool;
}


//	Agregar un nuevo trabajo al pool de hilos
int add_job_to_pool(ThreadPool* thread_pool, void (*function_p)(void*), void* arg_p)
{
	Job* newJobCreated;
	newJobCreated = (struct Job*) malloc(sizeof(struct Job));
	
	if (newJobCreated == NULL)
	{
		err("add_job_to_pool(): NO se puede obtener memoria para un nuevo trabajo\n");
		return -1;
	}

	// Se agrega la función y sus argumentos por ser realizado
	newJobCreated -> function = function_p;
	newJobCreated -> arg = arg_p;

	//	Se agrega el trabajo a la pila
	push_Job_Queue(&thread_pool->Job_Queue, newJobCreated);

	return 0;
}





//	Se destruye el pool de hilos
void destroy_ThreadPool(ThreadPool* thread_pool)
{
	if (thread_pool == NULL) return ;

	volatile int threads_total = thread_pool->num_threads_alive;

	//	Terminar los hilos en un ciclo infinito
	threads_keepalive = 0;

	//	Dar un segundo para matar los hilos ociosos
	double TIMEOUT = 1.0;
	time_t start, end;
	double time_passed = 0.0;
	time (&start);
	while (time_passed < TIMEOUT && thread_pool->num_threads_alive)
	{
		notify_ALL_by_semaphore(thread_pool->Job_Queue.has_jobs);
		time (&end);
		time_passed = difftime(end,start);
	}

	//	Concensuar los hilos faltantes
	while (thread_pool->num_threads_alive)
	{
		notify_ALL_by_semaphore(thread_pool->Job_Queue.has_jobs);
		sleep(1);
	}

	//	Se limpia la fila de trabajo
	destroy_Job_Queue(&thread_pool->Job_Queue);
	
	//	Destruir los hilos
	int n;
	for (n=0; n < threads_total; n++)
	{
		destroy_Thread(thread_pool->threads[n]);
	}
	free(thread_pool->threads);
	free(thread_pool);
}







/* ============================ HILO ============================== */


//	Iniciar un nuevo hili
static int init_Thread (ThreadPool* thread_pool, struct Thread** p_thread, int id)
{

	*p_thread = (struct Thread*)malloc(sizeof(struct Thread));
	if (p_thread == NULL)
	{
		err("init_Thread(): NO se puede obtener memoria para crear un hilo\n");
		return -1;
	}

	(*p_thread)->thread_pool = thread_pool;
	(*p_thread)->id       = id;

	pthread_create(&(*p_thread)->pthread, NULL, (void *)do_Thread, (*p_thread));
	pthread_detach((*p_thread)->pthread);
	return 0;
}


//	Se coloca el hilo en espera
static void hold_Thread(int sig_id) 
{
	threads_on_hold = 1;
	
	(void)sig_id;
	
	while (threads_on_hold)
	{
		sleep(1);
	}
}


//	Poner el hilo a trabajar
static void* do_Thread(struct Thread* thread_p)
{

	/* Assure all threads have been created before starting serving */
	ThreadPool* thread_pool = thread_p->thread_pool;

	/* Register signal handler */
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = hold_Thread;
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		err("do_Thread(): cannot handle SIGUSR1");
	}

	//	Se marca el hilo como "vivo"
	pthread_mutex_lock(&thread_pool->thread_count_lock);
	thread_pool->num_threads_alive += 1;
	pthread_mutex_unlock(&thread_pool->thread_count_lock);

	while(threads_keepalive)
	{

		wait_semaphore(thread_pool->Job_Queue.has_jobs);

		if (threads_keepalive)
		{

			pthread_mutex_lock(&thread_pool->thread_count_lock);
			thread_pool->num_threads_working++;
			pthread_mutex_unlock(&thread_pool->thread_count_lock);

			//	Se lee un trabajo de la fila y se ejecuta
			void (*func_buff)(void*);
			void*  arg_buff;
			Job* job_p = pull_Job_Queue(&thread_pool->Job_Queue);
			
			if (job_p) 
			{
				arg_buff  = job_p->arg;
				func_buff = job_p->function;
				func_buff(arg_buff);
				free(job_p);
			}

			pthread_mutex_lock(&thread_pool->thread_count_lock);
			thread_pool->num_threads_working--;
			
			if (!thread_pool->num_threads_working) 
			{
				pthread_cond_signal(&thread_pool->threads_all_idle);
			}
			pthread_mutex_unlock(&thread_pool->thread_count_lock);

		}
	}
	
	pthread_mutex_lock(&thread_pool->thread_count_lock);
	thread_pool->num_threads_alive --;
	pthread_mutex_unlock(&thread_pool->thread_count_lock);

	return NULL;
}


//	Se libera la memoria ocupada por el hilo
static void destroy_Thread (Thread* thread_p)
{
	free(thread_p);
}





/* ============================ FILA DE TRABAJO =========================== */


//	Se inicializa la fila de trabajo
static int init_Job_Queue(Job_Queue* jobqueue_p){
	jobqueue_p->len = 0;
	jobqueue_p->front_q = NULL;
	jobqueue_p->rear_q  = NULL;

	jobqueue_p->has_jobs = (struct B_semaphore*)malloc(sizeof(struct B_semaphore));
	
	if (jobqueue_p->has_jobs == NULL)
	{
		return -1;
	}

	pthread_mutex_init(&(jobqueue_p->rwmutex), NULL);
	init_semaphore(jobqueue_p->has_jobs, 0);

	return 0;
}


//	Se limpia la fila
static void clear_Job_Queue(Job_Queue* jobqueue_p)
{

	while(jobqueue_p->len)
	{
		free(pull_Job_Queue(jobqueue_p));
	}

	jobqueue_p->front_q = NULL;
	jobqueue_p->rear_q  = NULL;
	reset_semaphore(jobqueue_p->has_jobs);
	jobqueue_p->len = 0;
}


//	Agregar un trabajo a la pila
static void push_Job_Queue(Job_Queue* p_jobqueue, struct Job* newjob)
{

	pthread_mutex_lock(&p_jobqueue->rwmutex);
	newjob->previous = NULL;

	switch(p_jobqueue->len)
	{
		case 0:  
			p_jobqueue->front_q = newjob;
			p_jobqueue->rear_q  = newjob;
			break;
		default: 
			p_jobqueue->rear_q->previous = newjob;
			p_jobqueue->rear_q = newjob;

	}
	
	p_jobqueue->len++;

	notify_by_semaphore(p_jobqueue->has_jobs);
	pthread_mutex_unlock(&p_jobqueue->rwmutex);
}


//	Se obtiene el primer trabajo de la fila
static struct Job* pull_Job_Queue(Job_Queue* p_jobqueue)
{

	pthread_mutex_lock(&p_jobqueue->rwmutex);
	Job* job_p = p_jobqueue->front_q;

	switch(p_jobqueue->len)
	{
		case 0:
				break;
		case 1:  
			p_jobqueue->front_q = NULL;
			p_jobqueue->rear_q  = NULL;
			p_jobqueue->len = 0;
			break;
		default: 
			p_jobqueue->front_q = job_p->previous;
			p_jobqueue->len--;
			notify_by_semaphore(p_jobqueue->has_jobs);
	}

	pthread_mutex_unlock(&p_jobqueue->rwmutex);
	
	return job_p;
}


//	Se libera la memoria ocupada
static void destroy_Job_Queue(Job_Queue* jobqueue_p)
{
	clear_Job_Queue(jobqueue_p);
	free(jobqueue_p->has_jobs);
}





/* ======================== SINCRONIZACIÓN ========================= */


/* Init semaphore to 1 or 0 */
static void init_semaphore(B_semaphore *p_semaphore, int value) 
{
	if (value < 0 || value > 1) 
	{
		err("init_semaphore(): El semáforo binario solo puede tomar los valores 0 o 1\n");
		exit(1);
	}
	pthread_mutex_init(&(p_semaphore->mutex), NULL);
	pthread_cond_init(&(p_semaphore->cond), NULL);
	p_semaphore->v = value;
}


/* Reset semaphore to 0 */
static void reset_semaphore(B_semaphore *p_semaphore) 
{
	init_semaphore(p_semaphore, 0);
}


/* Post to at least one Thread */
static void notify_by_semaphore(B_semaphore *p_semaphore) 
{
	pthread_mutex_lock(&p_semaphore->mutex);
	p_semaphore->v = 1;
	pthread_cond_signal(&p_semaphore->cond);
	pthread_mutex_unlock(&p_semaphore->mutex);
}


/* Post to all threads */
static void notify_ALL_by_semaphore(B_semaphore *p_semaphore) 
{
	pthread_mutex_lock(&p_semaphore->mutex);
	p_semaphore->v = 1;
	pthread_cond_broadcast(&p_semaphore->cond);
	pthread_mutex_unlock(&p_semaphore->mutex);
}


/* Wait on semaphore until semaphore has value 0 */
static void wait_semaphore(B_semaphore* p_semaphore) 
{
	pthread_mutex_lock(&p_semaphore->mutex);
	while (p_semaphore->v != 1) {
		pthread_cond_wait(&p_semaphore->cond, &p_semaphore->mutex);
	}
	p_semaphore->v = 0;
	pthread_mutex_unlock(&p_semaphore->mutex);
}
