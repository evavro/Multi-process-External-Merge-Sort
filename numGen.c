#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[])
{
	srandomdev();
    int loop = atoi(argv[1]);
    
	while(loop > 0)
    {
        int ran = random() % 1000;
        printf("%i\n",ran);
        loop--;
    }
    
	return 0;	
}