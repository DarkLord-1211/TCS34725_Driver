#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>

#define TCS34725_IOCTL_MAGIC 't'
#define TCS34725_IOCTL_READ_CLEAR _IOR(TCS34725_IOCTL_MAGIC, 1, int)
#define TCS34725_IOCTL_READ_RED   _IOR(TCS34725_IOCTL_MAGIC, 2, int)
#define TCS34725_IOCTL_READ_GREEN _IOR(TCS34725_IOCTL_MAGIC, 3, int)
#define TCS34725_IOCTL_READ_BLUE  _IOR(TCS34725_IOCTL_MAGIC, 4, int)

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
}

int main() {
    int fd = open("/dev/tcs34725", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    signal(SIGINT, handle_sigint);  // Ctrl+C cleanup

    int clear, red, green, blue;

    printf("Reading TCS34725 color sensor (press Ctrl+C to stop)...\n");

    while (keep_running) {
        if (ioctl(fd, TCS34725_IOCTL_READ_CLEAR, &clear) < 0 ||
            ioctl(fd, TCS34725_IOCTL_READ_RED, &red) < 0 ||
            ioctl(fd, TCS34725_IOCTL_READ_GREEN, &green) < 0 ||
            ioctl(fd, TCS34725_IOCTL_READ_BLUE, &blue) < 0) {
            perror("Failed to read from device");
            break;
        }

        printf("\rClear: %-5d | Red: %-5d | Green: %-5d | Blue: %-5d", clear, red, green, blue);
        fflush(stdout);

        usleep(200000);  // 200ms delay
    }

    printf("\nExiting.\n");
    close(fd);
    return 0;
}
