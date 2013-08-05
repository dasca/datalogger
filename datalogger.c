
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <pthread.h>

#include "log.h"
#include "c_onewire.h"
#include "c_sqlite.h"

//Time betweens readings in seconds
#define LOG_INTERVAL    60

int KEEP_RUNNING;
int zExitCode;

int elapsedTicks;
int firstPulse = 1;
double elapsedkWh = 0.0, power = 0.0;
int ppwh = 1;
unsigned long lastTime = 0, pulseTime = 0;
unsigned long pulseCount = 0.0;
struct timeval tv;

void restoreGPIO() {
    FILE *handle = NULL;
    char buf[100];

    sprintf(buf, "/sys/class/gpio/unexport");
    if ((handle = fopen(buf, "w")) == NULL) {
        sprintf(log_buf, "Can´t open %s for manipulation", buf);
        log_entry(log_buf, EXIT_FAILURE);
    }
    fwrite("71", 2, 1, handle);
    fclose(handle);

}

void initGPIO() {
    FILE *handle = NULL;
    char buf[100];

    sprintf(buf, "/sys/kernel/debug/omap_mux/lcd_data1");
    if ((handle = fopen(buf, "r+")) == NULL) {
        sprintf(log_buf, "Can´t open %s for manipulation", buf);
        log_entry(log_buf, EXIT_FAILURE);
    }
    fwrite("37", 2, 1, handle);
    fclose(handle);
    handle = NULL;

    sprintf(buf, "/sys/class/gpio/export");
    if ((handle = fopen(buf, "w")) == NULL) {
        sprintf(log_buf, "Can´t open %s for manipulation", buf);
        log_entry(log_buf, EXIT_FAILURE);
    }
    fwrite("71", 2, 1, handle);
    fclose(handle);

    sprintf(buf, "/sys/class/gpio/gpio71/direction");
    if ((handle = fopen(buf, "r+")) == NULL) {
        sprintf(log_buf, "Can´t open %s for manipulation", buf);
        log_entry(log_buf, EXIT_FAILURE);
    }
    fwrite("in", 2, 1, handle);
    fclose(handle);

    sprintf(buf, "/sys/class/gpio/gpio71/edge");
    if ((handle = fopen(buf, "r+")) == NULL) {
        sprintf(log_buf, "Can´t open %s for manipulation", buf);
        log_entry(log_buf, EXIT_FAILURE);
    }
    fwrite("falling", 7, 1, handle);
    fclose(handle);
}

void intHandler() {
    sprintf(log_buf, "SIGINT caught, cleaning up");
    log_entry(log_buf, LOG_EXIT);
}

void onPowerPulse() {
    //used to measure time between pulses.
    gettimeofday(&tv, NULL);
    lastTime = pulseTime;
    pulseTime = 1000000 * tv.tv_sec + tv.tv_usec;

    if (firstPulse == 1) {
        firstPulse = 0;
        elapsedTicks = 0;
        power = 0;
        return;
    }

    //pulseCounter
    pulseCount++;

    //Calculate power
    power += (3600000000.0 / (pulseTime - lastTime)) / ppwh;

    //Find kwh elapsed
    elapsedkWh = (1.0 * pulseCount / (ppwh * 1000)); //multiply by 1000 to pulses per wh to kwh convert wh to kwh
    elapsedTicks++;
}

void *cPoll(void *notUsed) {
    struct pollfd fd[1];
    char buf[64];
    int i = 0, len = 0;
    fd[0].fd = open("/sys/class/gpio/gpio71/value", O_RDONLY);

    while (KEEP_RUNNING == 1) {

        fd[0].events = POLLPRI;
        i = poll(fd, 1, -1);

        if (i < 0) {
            log_entry("poll() failed!", LOG_EXIT);
        }

        if (fd[0].revents & POLLPRI) {
            len = read(fd[0].fd, buf, 64);
            onPowerPulse();
        }
    }

    return 0;
}

int log_device(struct sOneWireDevice *device_to_log) {
    return (addReading(device_to_log));
}

int main() {
    struct sOneWireDevice *devices, *device;
    pthread_t poll_thread;
    struct timeval tick;
    unsigned long last_tick;

#ifdef DEBUG
    fprintf(stderr, "Debug mode\r\n");
#endif

    log_entry("Starting up", LOG_NO_EXIT);
    KEEP_RUNNING = 1;
    zExitCode = 0;

    signal(SIGINT, intHandler);
    initDB();
    initGPIO();

    if (pthread_create(&poll_thread, NULL, cPoll, NULL)) {
        log_entry("Error creating thread", EXIT_FAILURE);
    }

    gettimeofday(&tick, NULL);
    last_tick = tick.tv_sec;

    devices = list_devices();

    while (KEEP_RUNNING == 1) {
        gettimeofday(&tick, NULL);
        if ((tick.tv_sec - last_tick) >= LOG_INTERVAL) {
            last_tick = tick.tv_sec;

            device = devices;
            while (device != NULL) {
                if (updateValue(device) != NULL)
                    log_device(device);
                device = device->next;
            }

            power = 0;
            elapsedTicks = 0;
        }
        sleep(1);
    }

    if (pthread_join(poll_thread, NULL)) {
        log_entry("Error joining thread\n", EXIT_FAILURE);
    }


    device = devices;
    while (device != NULL) {
        devices = device->next;
        free(device);
        device = devices;
    }
    restoreGPIO();
    log_entry("Cleanup finished, exiting", LOG_NO_EXIT);
    return (0);
}
