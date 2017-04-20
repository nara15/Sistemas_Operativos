/**
 * Archivo list_dir.c
 * Despliegue de una ruta en forma de árbol
 * 
 * Estudiante : José Mario Naranjo Leiva
 * Carnet :     2013034348
 **/
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/limits.h>

/**
 * Esta función determina si una ruta dada corresponde a un archivo
 * Entradas:
 *          p_ruta: const char (ruta que se desea verificar)
 * Salida:
 *          1 - es archivo
 *          0 - de lo contrario
 **/ 
int es_archivo(const char *p_ruta)
{
    struct stat path_stat;
    stat(p_ruta, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

/**
 * Esta función se encarga de verificar si la ruta es la ruta relativa . o ..
 * Entradas:
 *          p_ruta_nombre: const char (ruta que se desea verificar)
 * Salida:
 *      0 - es la ruta relativa 0 o 1. No se pueden expandir
 *      1 - sí se puede expandir la ruta.
 **/ 
int se_puede_expandir(const char *p_ruta_nombre)
{
    if ( !strcmp(".", p_ruta_nombre) || !strcmp("..", p_ruta_nombre))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * Esta función recursiva se encarga de ir mostrando recursivamente los archivos del directorio
 * y sus subdirectorios.
 * Entradas:
 *          p_nombreDir : const char (ruta que se desea listar).
 * Salida:
 *          imprime el árbol de visualización del directorio.
 **/
int listar_directorio(const char *p_nombreDir)
{
    struct dirent* dp;                   // Puntero a la entrada del directorio
    DIR *dir = opendir (p_nombreDir);   // Puntero al directorio.
    
    if (dir == NULL)
    {
        if (es_archivo(p_nombreDir))
        {
            struct stat st;
            stat(p_nombreDir, &st);
            int size = st.st_size;
            printf(" NOMBRE DE ARCHIVO: %s ---- Tamaño: %i bytes \n",p_nombreDir, size);
        }
        else
        {
            printf("No se pudo abrir el directorio\n");
        }
        
        return -1;
    }
    else
    {
        printf("%s \n",p_nombreDir);
        while ((dp = readdir(dir)) != NULL)
        {
            if (se_puede_expandir(dp->d_name))
            {

                char path[100];
                strcpy(path, p_nombreDir);
                strcat(path, "/");
                strcat(path, dp->d_name);

                listar_directorio(path);
            }
        }
    }
    
    closedir(dir);
    printf("%s ******************************* FIN \n",p_nombreDir);
    
}


// Entrada del programa.
int main()
{
    //Se está probando el directorio actual donde se encuentra este archivo.
    listar_directorio(".");
    
}
