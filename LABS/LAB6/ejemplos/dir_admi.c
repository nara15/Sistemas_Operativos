/*
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#define FOUND 0;
#define NOT_FOUND 1;
#define OPEN_ERROR 2;
#define READ_ERROR 4;


int main()
{
    DIR *dirp = opendir(".");
    struct dirent* dp;
    char name[256];
    
    while(dirp)
    {
        errno = 0;
        
        if ((dp = readdir(dirp)) != NULL)
        {
             printf("%s",d_name);
            if (strcmp(dp->d_name, name) == 0)
            {
                closedir (dirp);
               
                return FOUND;
            }
        }
        else
        {
            if (errno == 0)
            {
                closedir(dirp);
                return NOT_FOUND;
            }
            closedir(dirp);
            return READ_ERROR;
        }
    }
    
    return OPEN_ERROR;
}
*/


/*
#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <unistd.h>
#include <ftw.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#ifndef USE_FDS
#define USE_FDS 15
#endif

int print_entry(const char *filepath, const struct stat *info,
                const int typeflag, struct FTW *pathinfo)
{
 
    const double bytes = (double)info->st_size; // Not exact if large! 
    struct tm mtime;

    localtime_r(&(info->st_mtime), &mtime);

    printf("%04d-%02d-%02d %02d:%02d:%02d",
           mtime.tm_year+1900, mtime.tm_mon+1, mtime.tm_mday,
           mtime.tm_hour, mtime.tm_min, mtime.tm_sec);

    if (bytes >= 1099511627776.0)
        printf(" %9.3f TiB", bytes / 1099511627776.0);
    else
    if (bytes >= 1073741824.0)
        printf(" %9.3f GiB", bytes / 1073741824.0);
    else
    if (bytes >= 1048576.0)
        printf(" %9.3f MiB", bytes / 1048576.0);
    else
    if (bytes >= 1024.0)
        printf(" %9.3f KiB", bytes / 1024.0);
    else
        printf(" %9.0f B  ", bytes);

    if (typeflag == FTW_SL) {
        char   *target;
        size_t  maxlen = 1023;
        ssize_t len;

        while (1) {

            target = malloc(maxlen + 1);
            if (target == NULL)
                return ENOMEM;

            len = readlink(filepath, target, maxlen);
            if (len == (ssize_t)-1) {
                const int saved_errno = errno;
                free(target);
                return saved_errno;
            }
            if (len >= (ssize_t)maxlen) {
                free(target);
                maxlen += 1024;
                continue;
            }

            target[len] = '\0';
            break;
        }

        printf(" %s -> %s\n", filepath, target);
        free(target);

    } else
    if (typeflag == FTW_SLN)
        printf(" %s (dangling symlink)\n", filepath);
    else
    if (typeflag == FTW_F)
        printf(" %s\n", filepath);
    else
    if (typeflag == FTW_D || typeflag == FTW_DP)
        printf(" %s/\n", filepath);
    else
    if (typeflag == FTW_DNR)
        printf(" %s/ (unreadable)\n", filepath);
    else
        printf(" %s (unknown)\n", filepath);

    return 0;
}


int print_directory_tree(const char *const dirpath)
{
    int result;

    // Invalid directory path? 
    if (dirpath == NULL || *dirpath == '\0')
        return errno = EINVAL;

    result = nftw(dirpath, print_entry, USE_FDS, FTW_PHYS);
    if (result >= 0)
        errno = result;

    return errno;
}

int main(int argc, char *argv[])
{
    int arg;

    if (argc < 2) {

        if (print_directory_tree("../")) {
            fprintf(stderr, "%s.\n", strerror(errno));
            return EXIT_FAILURE;
        }

    } else {

        for (arg = 1; arg < argc; arg++) {
            if (print_directory_tree(argv[arg])) {
                fprintf(stderr, "%s.\n", strerror(errno));
                return EXIT_FAILURE;
            }
        }

    }

    return EXIT_SUCCESS;
}*/


#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

int is_directory_we_want_to_list(const char *parent, char *name) {
    struct stat st_buf;
    if (!strcmp(".", name) || !strcmp("..", name))
        return 0;

    char *path = malloc(strlen(name) + strlen(parent) + 2);
    sprintf(path, "%s/%s", parent, name);
    stat(path, &st_buf);
    return S_ISDIR(st_buf.st_mode);
}

int list(const char *name) {
    DIR *dir = opendir(name);
    struct dirent *ent;

    while (ent = readdir(dir)) {
        char *entry_name = ent->d_name;
        printf("%s\n", entry_name);

        if (is_directory_we_want_to_list(name, entry_name)) {
            // You can consider using alloca instead.
            char *next = malloc(strlen(name) + strlen(entry_name) + 2);
            sprintf(next, "%s/%s", name, entry_name);
            list(next);
            free(next);
        }
    }

    closedir(dir);
}

int main()
{
    list(".");
}