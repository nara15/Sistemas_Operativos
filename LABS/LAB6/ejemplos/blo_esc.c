
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    char* file = argv[1];   //El archivo para procesar
    int fd;                 //Descriptor del archivo
    struct flock lock;      //Bloqueador
    
    printf("abriendo %s\n", file);
    /* Abrir el descripto del archivo*/
    fd = open(file, O_WRONLY);
    printf("Bloqueando\n");
    
    /* Inicializar la estructura del flock*/
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    
    /* Colocar la cerradura de escritura al archivo*/
    fcntl (fd, F_SETLKW, &lock);
    
    printf("Bloqueado; dele enter para desbloquear\n");
    
    /*Esperar que usuario ingrese enter*/
    getchar();
    
    printf("Desbloqueado\n");
    
    /*Soltar la cerradura*/
    lock.l_type = F_UNLCK;
    fcntl (fd, F_SETLKW, &lock);
    
    close(fd);
    return 0;
}