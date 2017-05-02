//https://users.cs.cf.ac.uk/Dave.Marshall/C/node32.html

//http://www.cs.kent.edu/~ruttan/sysprog/lectures/multi-thread/thread-pool-server-with-join.c

#include <stdio.h>       
#define __USE_GNU 

#include <pthread.h>   

#include <stdlib.h>      


//========================= DEFINICIONES ==================================

//  Macro para la función de notificación de errores
#define err(str) fprintf(stderr, str)

// Número de threads usados en el pool
#define NUM_THREADS_IN_POOL 3

//  Mutex global para el pool
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

//  Varibla condición global
pthread_cond_t  got_job_request   = PTHREAD_COND_INITIALIZER;

//  Número de solicitudes pendientes
int num_requests = 0;

//  Bandera para indicar que no se van a crear más solicitudes de trabajo
int done_creating_jobRequests = 0;


//======================= ESTRUCTURAS ======================================

//  Estructura para controlar los parámetros de las funciones/trabajos
struct arg_structure
{
    int arg1;
    int arg2;
    
    // Parámetros para la lectura del archivo
    char* filename;
    int num_keys;
    char *keywords[6];
};

//  Estructura para manejar una solicitud de trabajo
struct Job 
{
    int job_id;                     //  Identificador del trabajo
    struct Job* next_job;           //  Puntero al próximo trabajo 
    void* args;     //  Argumento de la función
    void (*function)(void* args);    //  Puntero a la función
};

//  Cabeza de la lista enlazada de trabajos
struct Job* jobs_list = NULL;

//  Puntero a la última peticion de trabajo
struct Job* last_job    = NULL;


//====================== FUNCIONES ==========================================

/**
 *  @FUNCTION :
 *  @DESCRIPTION :
 *  @PARAMS :
 **/ 

void add_job_request(int p_num_job,
                     void (*p_function)(void*),
                     void* p_arg,
                     pthread_cond_t* p_condition_var,
                     pthread_mutex_t* p_mutex)
{
    struct Job* a_new_job;          //  Puntero al nuevo trabajo
    struct arg_structure *args  = p_arg;
    
    //  Se crea la estructura de la nueva petición
    
    a_new_job = (struct Job*) malloc(sizeof(struct Job));
    
    if (a_new_job == NULL)
    {
        err("add_job_request() : No hay más memoria\n");
        exit(1);
    }
    
    a_new_job -> job_id = p_num_job;
    a_new_job -> next_job = NULL;
    a_new_job -> function = p_function;
    a_new_job -> args = args;
    
    //  Bloquear el mutex para obtener acceso a la lista de jobs
    int rc;
    rc = pthread_mutex_lock(p_mutex);
    
    if (num_requests == 0)
    {
        jobs_list = a_new_job;
        last_job = a_new_job;
    }
    else
    {
        last_job -> next_job = a_new_job;
        last_job = a_new_job;
    }
    
    num_requests ++;
    
    //  Desbloquear el mutex
    rc = pthread_mutex_unlock(p_mutex);
    
    //  Notificar a la variable de notificación : hay una nueva petición de trabajo
    rc = pthread_cond_signal(p_condition_var);
    
}


/**
 *  @FUNCTION :
 *  @DESCRIPTION :
 *  @PARAMS :
 **/ 
struct Job* get_job_request(pthread_mutex_t* p_mutex)
{
    struct Job* the_job;
    
    int rc;
    rc = pthread_mutex_lock(p_mutex);
    
    if (num_requests > 0)
    {
        the_job = jobs_list;
        jobs_list = the_job -> next_job;
        
        if (jobs_list == NULL)
        {
            last_job = NULL;
        }
        num_requests -- ;
    }
    else
    {
        the_job = NULL;
    }
    
    rc = pthread_mutex_unlock(p_mutex);
    
    return the_job;
}



/**
 *  @FUNCTION :
 *  @DESCRIPTION :
 *  @PARAMS :
 **/
void doJob(struct Job* p_Job_to_do, int p_thread_ID)
{
    if (p_Job_to_do) 
    {
        
        void (*function_buffer)(void*);
        void* args_buff;
        
        args_buff = p_Job_to_do -> args;
        function_buffer = p_Job_to_do -> function;
        
        function_buffer (args_buff);
        
    	//printf("Thread '%d' handled request '%d'\n",p_thread_ID, p_Job_to_do -> job_id);
    	//fflush(stdout);
    }
}


/**
 *  @FUNCTION :
 *  @DESCRIPTION :
 *  @PARAMS :
 **/
void* handle_Job_Requests(void* data)
{
    struct Job* a_new_job;     
    int thread_id = *((int*)data); 

    int th;	   
    th = pthread_mutex_lock(&request_mutex);
    
    while (1)
    {
        if (num_requests > 0)
        { 
            a_new_job = get_job_request(&request_mutex);
            
            if (a_new_job)
            { 
            	doJob(a_new_job, thread_id);
            	free(a_new_job);
            }
        }
        else 
        {
            if (done_creating_jobRequests) 
            {
                pthread_mutex_unlock(&request_mutex);
                printf("hilo '%d' saliendo\n", thread_id);
                fflush(stdout);
                pthread_exit(NULL);
            }
            th = pthread_cond_wait(&got_job_request, &request_mutex);
        }
    }
}





