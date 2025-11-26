#include <sys/mman.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "../include/rwstdio.h"
#include "../include/shared_data.h"

#define RED_COLOR    "\x1b[31m" // child process output color
#define GREEN_COLOR  "\x1b[32m" // parent process output color
#define STANDARD_COLOR "\x1b[0m" // Standard color

int main() {
    // Создаём объект share memory
    int shm_fd = shm_open("/shm_data", O_RDWR | O_CREAT,  0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Размер разделяемой памяти
    if (ftruncate(shm_fd, sizeof(shared_data)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    // Отображаем разделяемую память в адресное пространство процесса
    shared_data *shared_mem = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Создание семафоров для синхронизации
    sem_t *sem_child1 = sem_open("/sem_child1", O_CREAT, 0666, 0);
    sem_t *sem_child2 = sem_open("/sem_child2", O_CREAT, 0666, 0);
    sem_t *sem_parent = sem_open("/sem_parent", O_CREAT, 0666, 0);
    if (sem_child1 == SEM_FAILED || sem_child2 == SEM_FAILED || sem_parent == SEM_FAILED) {
        perror("sem_open sem_child1");
        exit(EXIT_FAILURE);
    }

    // Создание процессов
    pid_t pid1, pid2;

    // Создание первого дочернего процесса
    pid1 = fork();
    // проверка на ошибки  
    if (pid1 == -1) {
        perror("fork error");
        exit(EXIT_FAILURE);
    }

    // child1 process
    if (pid1 == 0){
        // Замена текущего процесса на выполнение программы child1 с помощью execl
        // Если execl завершается с ошибкой, выводится сообщение и процесс завершается

        execl("./child1", "child1", NULL);
        perror("execl error");
        exit(EXIT_FAILURE);

    }

    // Создание второго дочернего процесса

    pid2 = fork();
    // проверка на ошибки
    if (pid2 == -1) {
        perror("fork error");
        exit(EXIT_FAILURE);
    }
    
    // child2 process
    if (pid2 == 0) {

        // Замена текущего процесса на выполнение программы child2 с помощью execl
        execl("./child2", "child2", NULL);
        perror("execl error");
        exit(EXIT_FAILURE);
    }

    else {
        // Чтение из стандартного ввода и запись в shm
        // В цикле читаем строки из stdin и пишем их в pipe1 с помощью write_all
        // Если write_all возвращает -1, выводим сообщение об ошибке, освобождаем память и выходим из программы с ошибкой
        char *buffer = NULL;
        size_t len = 0;
        buffer = read_all_fd(STDIN_FILENO, &len);
        if (buffer == NULL) {
            perror("Ошибка чтения из stdin");
            return 1;
        }
        shared_mem->size = len;
        memcpy(shared_mem->buffer, buffer, len);
        free(buffer);

        sem_post(sem_child1);  // Разрешаем первому ребёнку читать из shm

        sem_wait(sem_parent);  // Ждём, пока оба ребёнка закончат обработку

        // Чтение из shm и вывод в стандартный вывод
        if (write_all_fd(STDOUT_FILENO, shared_mem->buffer, (size_t)shared_mem->size) == -1) {
                perror("write to stdout");
            }

        
        // Ожидание завершения дочерних процессов
        // Это очень важно, чтобы избежать зомби-процессов, про которые написано в materials.md
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);

        // Очистка ресурсов
        // Закрытие и удаление семафоров, а также удаление разделяемой памяти
        munmap(shared_mem, sizeof(shared_data));
        shm_unlink("shm_data");
        sem_close(sem_child1);
        sem_close(sem_child2);
        sem_close(sem_parent);
        sem_unlink("/sem_child1");
        sem_unlink("/sem_child2");
        sem_unlink("/sem_parent");
    }


    return 0;
}