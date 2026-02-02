#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <locale.h>
#include <stdlib.h>


int main(int argc, char* argv[])
{
        setlocale(LC_ALL, "pl_PL.UTF-8");
        char *buf = (char*)malloc(4096 * sizeof(char));
        if (!buf)
        {
                fprintf(stderr, "malloc failed!\n");
                exit(EXIT_FAILURE);
        }
        /*for(int i=1; i<argc; i++)
        {
                printf("Processing file %d/%d: %s\n", i, argc-1, argv[i]);
        }*/
        for(int i=1; i<argc; i++)
        {
                //printf("Processing file %d/%d: %s\n", i, argc-1, argv[i]);
                int fd = open(argv[i], O_RDONLY);
                int n, znakow=0, linii=0;
                //n=lseek(fd, 0, SEEK_SET);
                //printf("\n%d\n", n);//0
                if (fd == -1)
                {
                        printf("Kod: %d\n", errno);
                        perror("Otwarcie pliku");
                        exit(1);
                }
                //printf("%d", (n=read(fd, buf, 20)));
                while((n=read(fd, buf, 4096)) > 0)
                {
                        //printf("\n%d\n", n);
                         //write(1, buf, n);
                        znakow+=n;
                         //printf("\n%d\n", znakow);
                        for (int i=0; i<n; i++)
                        {
                                if (buf[i]=='\n')
                                {
                                        linii++;
                                }
                        }
                }
                //printf("\n%d\n", n);//-1
                //write(1, "bulem B.K.", 10);
                printf("\n%s: %d znaków  ", argv[i], znakow);
                //n=lseek(fd, -1, SEEK_END);
                //printf("\n%d\n", n);//182
                printf("  %d\n", linii);
                close(fd);
        }
        return 0;
}
