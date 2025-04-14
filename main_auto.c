// main_auto.c
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
struct timespec start_time;

void log_error(const char *msg) {
    FILE *log = fopen("main_log.txt", "a");
    if (log) {
        time_t now = time(NULL);
        fprintf(log, "[%s] ERROR: %s\n", strtok(ctime(&now), "\n"), msg);
        fclose(log);
    }
}

void log_info(const char *msg) {
    FILE *log = fopen("main_log.txt", "a");
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
    struct can_frame frame;
    char buf[64] = {0};

    while (1) {
        if (read(sock, &frame, sizeof(frame)) > 0) {
            memcpy(buf, frame.data, frame.can_dlc);
            printf("\n[Received] From Module -> ID:%d -> %s\n", frame.can_id, buf);
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "Received from Module ID:%d -> %s", frame.can_id, buf);
            log_info(log_msg);
        }
    }
    return NULL;
}

int main() {
    setup_can_interface();
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    pthread_t recv_thread;

    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        log_error("Socket creation failed.");
        return 1;
    }

    strcpy(ifr.ifr_name, "can0");
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        log_error("ioctl failed to get interface index.");
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

    int module_ids[] = {11, 13, 59};
    int current = 0;

    while (1) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        int elapsed_ms = (now.tv_sec - start_time.tv_sec) * 1000 +
                         (now.tv_nsec - start_time.tv_nsec) / 1000000;

        if (elapsed_ms >= 10) {
            start_time = now; // Reset base time every 200ms

            int target = module_ids[current];
            char msg[8];
            snprintf(msg, sizeof(msg), "M%d", target);

            frame.can_id = target;
            frame.can_dlc = strlen(msg);
            memcpy(frame.data, msg, frame.can_dlc);

            if (write(sock, &frame, sizeof(frame)) != sizeof(frame)) {
                perror("Write");
                log_error("Failed to send CAN frame to module.");
            } else {
                printf("[Sent] To Module %d -> %s\n", target, msg);
                char log_msg[64];
                snprintf(log_msg, sizeof(log_msg), "Sent to Module %d -> %s", target, msg);
                log_info(log_msg);
            }

            current = (current + 1) % 3;
        }

        usleep(10000);  // 10ms check
    }

    close(sock);
    return 0;
}
