
#include <stdio.h>

int main()
{
    int aInt = 368;
    char str[15];
    sprintf(str, "%d", aInt);
    
    printf("%s",str);
}