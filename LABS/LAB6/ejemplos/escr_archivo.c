// http://www.makelinux.net/alp/098
#include <fcntl.h> 
#include <stdio.h> 
#include <string.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <time.h> 
#include <unistd.h> 


char* get_timestamp()
{
    time_t now = time(NULL);
    return asctime (localtime (&now));
}


int main(int argc, char* argv[])
{
    /* El archivo al cuál agregarle la impresión del tiempo*/
    char* filename = argv[1];
    /* Obtener la hora actual */
    char* timestamp = get_timestamp();
    /*Abrir el archivo. Si existe, agregue; sino, cree un nuevo archivo*/
    int fd = open (filename, O_WRONLY | O_CREAT | O_APPEND);
    /* Obtener el largo de la hilera obtenida*/
    size_t length = strlen(timestamp);
    /* Escribir en el archivo */
    write(fd, timestamp, length);
    
    /*Todo realizado*/
    close (fd);
    return 0;
}