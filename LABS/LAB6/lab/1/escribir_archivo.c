/**
 * Archivo escribir_archivo.c
 * Escribe sobre un archivo con bloqueo para cad proceso de escritura
 * 
 * Estudiante : José Mario Naranjo Leiva
 * Carnet :     2013034348
 **/

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>



#define NUM_THREADS     2
#define NUM_TOTAL       10
#define NUM_GROUP       5

int contIMPAR = 1;
int contPAR = 0;


void escribir_archivo(char *buffer)
{
    char* file = "pares_impares.txt";   //El archivo para procesar
    int fd;                 //Descriptor del archivo
    struct flock lock;      //Bloqueador
    
  
    /* Abrir el descripto del archivo*/
    fd = open(file, O_WRONLY | O_CREAT | O_APPEND);
  
    /* Inicializar la estructura del flock*/
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    
    /* Colocar la cerradura de escritura al archivo*/
    fcntl (fd, F_SETLKW, &lock);
    
    // TRATAR DE ESCRIBIR ALGO
    /* Obtener el largo de la hilera obtenida*/
    size_t length = strlen(buffer);
    /* Escribir en el archivo */
    write(fd, buffer, length);
    
    /*Soltar la cerradura*/
    lock.l_type = F_UNLCK;
    fcntl (fd, F_SETLKW, &lock);
    
    close(fd);
}

void *escribir_impar(void *threadid)
{
    long tid;
    tid = (long)threadid;
    char buffer[15] = "";
    //printf("It's me IMPAR, thread #%ld!\n", tid);
    
    while (contIMPAR < NUM_TOTAL)
    {
        if (contIMPAR + 2 <= NUM_GROUP)
        {
            int i = contIMPAR;
            
            while (i < NUM_GROUP)
            {
                contIMPAR = contIMPAR + 2;
                
                char str[15];
                sprintf(str, "%d", contIMPAR);
                strcat(buffer, "--");
                strcat(buffer, str);
                //printf("Contador IMPAR %i \n",contIMPAR);
                i++;
            
            }
            escribir_archivo(buffer);
            printf("%s\n", buffer);
            sleep(1);
        }
        else
        {
            break;
        }
    }
}

void *escribir_par(void *threadid)
{
    long tid;
    tid = (long)threadid;
    //printf("It's me PAR, thread #%ld!\n", tid);
    
    char buffer[15] = "";
    
    while (contPAR < NUM_TOTAL)
    {
        if (contPAR + 2 <= NUM_GROUP)
        {
            int i = contPAR;
            
            while (i < NUM_GROUP)
            {
                contPAR = contPAR + 2;
                
                char str[15];
                sprintf(str, "%d", contPAR);
                strcat(buffer, "--");
                strcat(buffer, str);
                
                //printf("Contador PAR %i \n",contPAR);
                i++;
            
            }
            escribir_archivo(buffer);
            printf("%s\n", buffer);
            sleep(1);
        }
        else
        {
            break;
        }
    }
}


int main (int argc, char *argv[])
{
   pthread_t threads[NUM_THREADS];
   int rc,rc1;
   long t;
   for(t=0; t<NUM_THREADS; t++){
      printf("In main: creating thread %ld\n", t);
      rc = pthread_create(&threads[t], NULL, escribir_impar, (void *)t);
      rc1 = pthread_create(&threads[t], NULL, escribir_par, (void *)t);
      if (rc){
         printf("ERROR; return code from pthread_create() is %d\n", rc);
         //exit(-1);
      }
   }
   pthread_exit(NULL);
}