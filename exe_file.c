#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void shuffle(int *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;

        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}


int main(){

    char name;
    int  i, vec[256];
    FILE *f;
    int check = 0;

    f = fopen("auth.txt", "w+");

	for(i=0; i<256; i++)
    {
    	vec[i] = i;

    }

	shuffle(vec, 256);
	
	for(i=0; i<256; i++)
    {
    	fprintf(f,"%d\n",vec[i]);

    }

    fclose(f);

    return 0;
}

