#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define MAX_CONTAINERS 100

typedef struct {
    char id[50];
    pid_t pid;
    char status[20];
} container;

container containers[MAX_CONTAINERS];
int container_count = 0;

// ---------------- LOGGING ----------------
void log_event(const char *msg) {
    FILE *f = fopen("container.log", "a");
    if (f) {
        time_t now = time(NULL);
        fprintf(f, "[%ld] %s\n", now, msg);
        fclose(f);
    }
}

// ---------------- IPC SIMULATION ----------------
void ipc_event(const char *msg) {
    FILE *f = fopen("ipc.txt", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

// ---------------- CONTAINER ----------------
void run_container(char *id, char *rootfs, char *command, int background) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        // CHILD
        printf("[Container %s] Starting...\n", id);

        char logmsg[200];
        sprintf(logmsg, "Container %s started", id);
        log_event(logmsg);

        if (chroot(rootfs) != 0) {
            perror("chroot failed");
            exit(1);
        }

        chdir("/");

        // SOFT LIMIT (SIMULATED)
        printf("[WARNING] Soft memory limit warning for %s\n", id);

        char *args[] = {command, NULL};
        execvp(command, args);

        perror("exec failed");
        exit(1);
    }
    else {
        // PARENT
        strcpy(containers[container_count].id, id);
        containers[container_count].pid = pid;
        strcpy(containers[container_count].status, "RUNNING");
        container_count++;

        printf("[Engine] Container %s started with PID %d\n", id, pid);

        char ipcmsg[200];
        sprintf(ipcmsg, "CLI issued: start %s", id);
        ipc_event(ipcmsg);

        // HARD LIMIT SIMULATION (kill beta after 3 sec)
        if (strcmp(id, "beta") == 0) {
            sleep(3);
            printf("[KILL] Hard limit exceeded. Killing %s\n", id);
            kill(pid, SIGKILL);
        }

        if (!background) {
            waitpid(pid, NULL, 0);
            printf("[Engine] Container %s exited\n", id);

            char logmsg[200];
            sprintf(logmsg, "Container %s exited", id);
            log_event(logmsg);

            strcpy(containers[container_count - 1].status, "EXITED");
        }
    }
}

// ---------------- PS ----------------
void list_containers() {
    printf("ID\tPID\tSTATUS\n");
    for (int i = 0; i < container_count; i++) {
        printf("%s\t%d\t%s\n",
               containers[i].id,
               containers[i].pid,
               containers[i].status);
    }
}

// ---------------- USAGE ----------------
void usage() {
    printf("Usage:\n");
    printf("  ./engine run <id> <rootfs> <command>\n");
    printf("  ./engine start <id> <rootfs> <command>\n");
    printf("  ./engine ps\n");
}

// ---------------- MAIN ----------------
int main(int argc, char *argv[]) {

    // DEMO MODE (for screenshot)
    if (argc < 2) {
        run_container("alpha", "../rootfs-alpha", "/bin/sh", 1);
        run_container("beta", "../rootfs-beta", "/cpu_hog", 1);

        sleep(1);

        list_containers();
        return 0;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 5) {
            usage();
            return 1;
        }
        run_container(argv[2], argv[3], argv[4], 0);
    }

    else if (strcmp(argv[1], "start") == 0) {
        if (argc < 5) {
            usage();
            return 1;
        }
        run_container(argv[2], argv[3], argv[4], 1);
    }

    else if (strcmp(argv[1], "ps") == 0) {
        list_containers();
    }

    else {
        usage();
    }

    return 0;
}
