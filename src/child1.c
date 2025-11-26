#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // STDIN_FILENO, STDOUT_FILENO
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../include/string_to_lowercase.h"
#include "../include/rwstdio.h"
#include "../include/shared_data.h"

int main(void) {

    // Открываем объект разделяемой памяти
    int shm_fd = shm_open("/shm_data", O_RDWR,  0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Читаем весь shm в динамический буфер
    shared_data *shm = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Открываем семафоры
    sem_t *sem_child1 = sem_open("/sem_child1", 0);
    sem_t *sem_child2 = sem_open("/sem_child2", 0);
    if (sem_child1 == SEM_FAILED || sem_child2 == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
    sem_wait(sem_child1);  // Ждём данные от parent

    // Обрабатываем данные из shm
    string_to_lowercase(shm->buffer);
    
    // Сигнализируем child2, что данные готовы
    sem_post(sem_child2);

    // Закрываем семафоры
    sem_close(sem_child1);
    sem_close(sem_child2);
    munmap(shm, sizeof(shared_data));
    close(shm_fd);


    return 0;
}