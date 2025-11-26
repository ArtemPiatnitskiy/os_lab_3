#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // STDIN_FILENO, STDOUT_FILENO
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../include/space_replace.h"
#include "../include/rwstdio.h"
#include "../include/shared_data.h"

int main(void) {

    // Открываем объект разделяемой памяти
    int shm_fd = shm_open("/shm_data", O_RDWR,  0666);

    // Читаем весь shm в динамический буфер
    shared_data *shm = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Открываем семафоры
    sem_t *sem_child2 = sem_open("/sem_child2", 0);
    sem_t *sem_parent = sem_open("/sem_parent", 0);
    
    sem_wait(sem_child2);  // Ждём данные от child1

    // Обрабатываем данные из shm
    space_replace(shm->buffer);
    
    // Сигнализируем parent, что данные готовы
    sem_post(sem_parent);

    // Закрываем семафоры
    sem_close(sem_child2);
    sem_close(sem_parent);
    munmap(shm, sizeof(shared_data));
    close(shm_fd);

    
    return 0;
}