#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

int main(int argc, char **argv)
{
   struct timeval tv;

   tv.tv_sec = atoi(argv[1]);
   tv.tv_usec = 0;

   settimeofday(&tv, NULL);
   gettimeofday(&tv, NULL);

   printf("%d %d\n", tv.tv_sec, tv.tv_usec);
   return 0;

}
