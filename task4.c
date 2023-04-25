#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>

#define SEM_NAME "/sem"

int main(int argc, char *argv[]) { // fisrt arg - cnt_proc
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    int cnt_proc = strtoll(argv[1], NULL, 10);

    if (cnt_proc <= 0) {
        return 0;
    }

    int *cnt_call = mmap(NULL, cnt_proc, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (cnt_call == MAP_FAILED) {
        perror("memory failed");
        exit(1);
    }

    pid_t *sons = malloc(cnt_proc * sizeof(sons));
    sem_t **sem = malloc(cnt_proc * sizeof(*sem));

    for (int i = 0; i < cnt_proc; ++i) {
        char *path[100];
        sprintf(path, "%s%d", SEM_NAME, i);
        sem[i] = sem_open(path, O_CREAT, 0644, 0); // Создание именованного семафора
        if (sem[i] == SEM_FAILED) {
            printf("failed sem_open");

            // close inited sems
            for (int j = i - 1; j > -1; --j) {
                sem_close(sem[j]);
            }
            // close inited sems

            exit(1);
        }

    }
    for (int i = 0; i < cnt_proc; ++i) {
        cnt_call[i] = 0;
    }

    for (int i = 0; i < cnt_proc; ++i) {
        sons[i] = fork();
        if (!sons[i]) {
            srand(i);
            int state = rand() % 2;
            if (!state) { // if it waits call
                sem_post(sem[i]);
            }
            while (1) {
                if (state) { // it wants to call
                    printf("%d %s", i, "будет звонить\n");
                    int want_to_call = rand() % cnt_proc;
                    int check = -1;
                    while (want_to_call == i || check == -1) {
                        want_to_call = rand() % cnt_proc;
                        if (want_to_call != i) {
                            check = sem_trywait(sem[want_to_call]);
                        }
                    }
                    cnt_call[i] += 1;
                    cnt_call[want_to_call] += 1;
                    printf("%d %s %d %s", i, "позвонил", want_to_call, "\n");
                    // TALKING
                    sleep(10);
                    printf("%d %s %d %s", i, "завершился", want_to_call, "\n");
                    // TALKING
                    // call finished
                } else { // it waits for call
                    printf("%d %s", i, "будет ждать звонка\n");
                    int own_value;
                    while (1) { // process of waiting call
                        sem_getvalue(sem[i], &own_value);
                        if (own_value == 0) {
                            // TALKING
                            printf("%d %s", i, "болтает\n");
                            sleep(10);
                            printf("%d %s %s", i, "завершился", "\n");
                            // TALKING
                            break; // stop waiting call
                        }
                    }
                }

                // change state
                state = rand() % 2;
                if (!state) { // if it waits for call
                    sem_post(sem[i]); // increase semaphore
                }
            }
        }
    }

    // time for program execution
    int exec_time = 15;
    sleep(exec_time);
    // time for program execution

    for (int i = 0; i < cnt_proc; i++) {
        kill(sons[i], SIGKILL);
        wait(NULL);
    }
    for (int i = 0; i < cnt_proc; ++i) {
        sem_close(sem[i]);
    }

    free(sem);
    free(sons);

    for (int i = 0; i < cnt_proc; ++i) {
        printf("%d cnt: %d \n", i, cnt_call[i]);
    }
    munmap(cnt_call, cnt_proc);

    for (int i = 0; i < cnt_proc; ++i) {
        char *path[100];
        sprintf(path, "%s%d", SEM_NAME, i);
        sem_unlink(path);
    }
    return 0;
}
