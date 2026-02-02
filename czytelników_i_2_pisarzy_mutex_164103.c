//#dodanie bibliotek

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include "error.h"

//deklaracje zmiennych

int ile_czytaczy=10, ile_pisarzy=2;
int ile_czyta = 0;
bool verbose;

pthread_mutex_t zasobMutex;
pthread_mutex_t czytelMutex;
char *bufor="co zapisano";

//procedury procesów
void *pisarz()
{
        while (1)
        {
                pthread_mutex_lock(&zasobMutex);
                printf("zapisuje");
                            // Odbywa się pisanie
                pthread_mutex_unlock(&zasobMutex);
        }
}

void *czytelnik()
{
        while (1)
        {
                pthread_mutex_lock(&czytelMutex);
                ile_czyta++;//czytelnik więcej
                if (ile_czyta == 1)//blokujemy jeśli zasoby zaczeły być potrzebne
                {
                        pthread_mutex_lock(&zasobMutex);
                }

                pthread_mutex_unlock(&czytelMutex);
                printf("odczytano: %s", bufor);           //odbywa się czytanie
                pthread_mutex_lock(&czytelMutex);

                ile_czyta--;//czytelnik mniej
                if (ile_czyta == 0)//odblokowujemy jeśli zasoby przestały być potrzebne
                {
                        pthread_mutex_unlock(&zasobMutex);
                }
                pthread_mutex_unlock(&czytelMutex);
        }
}



int main()
{
        pthread_t *readersThreadsIds = malloc(ile_czytaczy * sizeof(pthread_t));
        if (readersThreadsIds == NULL)
        {
                perror("cannotAllocateMemoryError()");
        }
        pthread_t *writersThreadsIds = malloc(ile_pisarzy * sizeof(pthread_t));
        if (writersThreadsIds == NULL)
        {
                perror("cannotAllocateMemoryError()");
        }

        for (int i = 0; i < ile_czytaczy; i++)
        {
                if (pthread_create(&readersThreadsIds[i], NULL, czytelnik, (void *) (uintptr_t) i) != 0)
                {
                        perror("pthread_create error");
                }
        }
        for (int i = 0; i < ile_pisarzy; i++)
        {
                if (pthread_create(&writersThreadsIds[i], NULL, pisarz, NULL) != 0)
                {
                        perror("pthread_create error");
                }
        }
        //uruchomienie procesów
        for (int i = 0; i < ile_czytaczy; i++)
        {
                if (pthread_join(readersThreadsIds[i], NULL) != 0)
                {
                        perror("pthread_join error");
                }
        }
        for (int i = 0; i < ile_pisarzy; i++)
        {
                if (pthread_join(writersThreadsIds[i], NULL) != 0)
                {
                        perror("pthread_join error");
                }
        }

        free(readersThreadsIds);
        free(writersThreadsIds);
        pthread_mutex_destroy(&zasobMutex);
        pthread_mutex_destroy(&czytelMutex);
        return 0;
}
