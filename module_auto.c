// module_auto.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

int sock;
int module_id;
char log_filename[32];
struct timespec start_time;

void log_error(const char *msg) {
    FILE *log = fopen(log_filename, "a");
    if (log) {
        time_t now = time(NULL);
        fprintf(log, "[%s] ERROR: %s\n", strtok(ctime(&now), "\n"), msg);
        fclose(log);
    }
}

void log_info(const char *msg) {
    FILE *log = fopen(log_filename, "a");
    if (log) {
        time_t now = time(NULL);
        fprintf(log, "[%s] INFO: %s\n", strtok(ctime(&now), "\n"), msg);
        fclose(log);
    }
}

void setup_can_interface() {
    system("ip link set can0 down");
    system("ip link set can0 type can bitrate 50000");
    system("ip link set can0 up");
}

void *receive_data(void *arg) {
    struct can_frame rcv;
    while (1) {
        if (read(sock, &rcv, sizeof(rcv)) > 0) {
            if (rcv.can_id != module_id) continue;

            char buf[64] = {0};
            memcpy(buf, rcv.data, rcv.can_dlc);

            printf("\n[Received] From MAIN -> %s\n", buf);
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "Received from MAIN -> %s", buf);
            log_info(log_msg);
        }
    }
    return NULL;
}

void *auto_send(void *arg) {
    struct can_frame frame;
    int slot = (module_id == 11) ? 0 : (module_id == 13) ? 1 : 2;

    while (1) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        int elapsed_ms = (now.tv_sec - start_time.tv_sec) * 1000 +
                         (now.tv_nsec - start_time.tv_nsec) / 1000000;

        int slot_now = (elapsed_ms / 10) % 3;

        if (slot_now == slot) {
            char msg[8];
            snprintf(msg, sizeof(msg), "D%d", module_id);

            frame.can_id = 7;
            frame.can_dlc = strlen(msg);
            memcpy(frame.data, msg, frame.can_dlc);

            if (write(sock, &frame, sizeof(frame)) != sizeof(frame)) {
                perror("Send auto");
                log_error("Auto message send failed.");
            } else {
                printf("[Auto-Sent] To MAIN -> %s\n", msg);
                char log_msg[64];
                snprintf(log_msg, sizeof(log_msg), "Auto-Sent to MAIN -> %s", msg);
                log_info(log_msg);
            }

            usleep(200000);  // sleep rest of slot
        } else {
            usleep(50000);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <module_id>\n", argv[0]);
        return 1;
    }

    module_id = atoi(argv[1]);
    snprintf(log_filename, sizeof(log_filename), "module_log%d.txt", module_id);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    struct sockaddr_can addr;
    struct ifreq ifr;
    pthread_t recv_thread, auto_thread;

    setup_can_interface();

    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        log_error("Socket creation failed.");
        return 1;
    }

    strcpy(ifr.ifr_name, "can0");
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        log_error("Failed to get CAN index.");
        return 1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        log_error("Socket bind failed.");
        return 1;
    }

    pthread_create(&recv_thread, NULL, receive_data, NULL);
    pthread_create(&auto_thread, NULL, auto_send, NULL);

    printf("Module %d is active.\n", module_id);

    struct can_frame frame;
    char msg[64];

    while (1) {
        printf("Enter message to MAIN (press Enter to skip): ");
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = 0;

        if (strlen(msg) == 0) continue;

        frame.can_id = 7;
        frame.can_dlc = strlen(msg);
        memcpy(frame.data, msg, frame.can_dlc);

        if (write(sock, &frame, sizeof(frame)) != sizeof(frame)) {
            perror("Manual send");
            log_error("Manual message send failed.");
        } else {
            printf("[Manual Sent] -> %s\n", msg);
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "Manual Sent -> %s", msg);
            log_info(log_msg);
        }
    }

    close(sock);
    return 0;
}
