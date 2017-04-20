#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

int is_directory(const char *p_ruta)
{
    struct stat statbuf;
    if (stat(p_ruta, &statbuf) != 0) return 0;
    return S_ISDIR(statbuf.st_mode);
}


int listar_directorio(const char *p_nombreDir)
{
    struct dirent* dp;                   // Puntero a la entrada del directorio
    DIR *dir = opendir (p_nombreDir);   // Puntero al directorio.
    
    if (dir == NULL)
    {
        printf("No se pudo abrir el directorio");
        return -1;
    }
    
    while ((dp = readdir(dir)) != NULL)
    {
        printf("%s\n", dp->d_name);
        if ( is_directory(dp->d_name))
        {
            //printf("%s\n", dp->d_name);
        }
    }
    
    closedir(dir);
}

int main()
{
    listar_directorio("../../LAB6");
}

/*
int main(void)
{
    struct dirent *de;  // Pointer for directory entry

    // opendir() returns a pointer of DIR type. 
    DIR *dr = opendir(".");

    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory" );
        return 0;
    }

    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
    // for readdir()
    while ((de = readdir(dr)) != NULL)
            printf("%s\n", de->d_name);

    closedir(dr);    
    return 0;
}*/