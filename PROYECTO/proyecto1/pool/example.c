#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "threadpool.h"




int keyword_process(char* line, char* keyword) {
    int count;
    size_t nlen = strlen (keyword);

    while (line != NULL) {
      line = strstr (line, keyword);
      if (line != NULL) {
        count++;
        line += nlen;
      }
    }
    return count;
}

void line_process(char* line, int line_id, char *keywords[], int num_keys, int *counters) {

    int i;
    for (i = 0; i<num_keys; i++)
        counters[i*500+line_id] = keyword_process(line,keywords[i]);
}

void print_bar(char *keywords[], int num_keys, int num_line, int* counters) {
    int j;
    for (j=0; j<num_keys; j++)
    {
        printf("%-15s",keywords[j]);
        int i;
        for (i=0; i<num_line; i++)
        {
            if (counters[j*500+i]==0)
                printf("\u2591");
            else if (counters[j*500+i]==1)
                printf("\u2592");
            else if (counters[j*500+i]==2)
                printf("\u2593");
            else
                printf("\u2589");
        }
        printf("\n");
    }
    printf("\n\n");
}


void* file_process(void* p_args)
{
    struct arg_structure *args = (struct arg_structure *) p_args;
   
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int counters[500*args->num_keys];

    fp = fopen(args -> filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    int num_line=0;
    
    while ((read = getline(&line, &len, fp)) != -1) {
        
        if (len<80) continue;
        line_process(line, num_line, args -> keywords, args -> num_keys, &counters);
        
        num_line++;
    }

    print_bar( args -> keywords, args -> num_keys, num_line, counters);
   

    fclose(fp);
    if (line)
        free(line);

}





int main(int argc, char* argv[])
{
    
     struct arg_structure args;
    
    // PROCESAR ENTRADAS -------------------------------------------------------
    char *keywords[6];

    if (argc == 1) 
    {
        printf("No files specified\n");
        exit(EXIT_FAILURE);
    }
    clock_t tic = clock();

    int num_keys = 0;
    char *token = strtok(argv[1], ",");
    while (token != NULL && num_keys < 6) 
    {
        int size = strlen(token);
        args.keywords[num_keys] = malloc(sizeof(char) * size + 1);
        strncpy(args.keywords[num_keys], token, size + 1);
        token = strtok(NULL, ",");
        num_keys++;
    }
    
    // HILOS -------------------------------------------------------------------
    int  i;                               
    int thr_id[NUM_THREADS_IN_POOL];      
    pthread_t  threadPool[NUM_THREADS_IN_POOL];   
    struct timespec delay;	
   

    for (i = 0; i < NUM_THREADS_IN_POOL; i ++)
    {
    	thr_id[i] = i;
    	pthread_create(&threadPool[i], NULL, handle_Job_Requests, (void*)&thr_id[i]);
    }

 
    for (i = 2; i < argc; i ++) 
    {
        args.filename = argv[i];
        args.num_keys = num_keys;
       
        add_job_request(i, (void*) file_process, (void *)&args, &got_job_request, &request_mutex);
    
    	if (rand() > 3*(RAND_MAX / 4)) 
    	{ 
    	    delay.tv_sec = 0;
    	    delay.tv_nsec = 1;
    	    nanosleep(&delay, NULL);
    	}
    }

    //  Se avisa al manejador que ya no se van a generar más trabajos 
    {
        int rc;
        rc = pthread_mutex_lock(&request_mutex);
        done_creating_jobRequests = 1;
        rc = pthread_cond_broadcast(&got_job_request);
        rc = pthread_mutex_unlock(&request_mutex);
    }


    //  Se debe esperar que ya no hayan más trabajos en la cola y que los hilos acaben
    for (i = 0; i < NUM_THREADS_IN_POOL; i ++) 
    {
    	void* thr_retval;
    	pthread_join(threadPool[i], &thr_retval);
    }

    
    return 0;
}
