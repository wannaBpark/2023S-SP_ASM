#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIN 20
#define POUT2 17
#define VALUE_MAX 40

static int order[6][3] = { {0,1,2}, {0,2,1},{1,0,2},{1,2,0},{2,1,0},{2,0,1} };
static int visited[3];
static int guess[3];
static int LED[3] = { 26,19,13 };
static int Button[3] = { 16,20,21 };
static enum Result {
    SUCCESS = 6,
    FAIL = 5
};

static int GPIORead(int pin);
static int GPIOExport(int pin);
static int GPIODirection(int pin, int dir);
static int GPIOWrite(int pin, int value);
static int GPIOUnexport(int pin);
static void PrintPinSec(int pin, float sec)
{
    GPIOWrite(pin, 1);
    usleep(1000000 * sec);
    GPIOWrite(pin, 0);
    usleep(1000000 * sec);
}
static void PrintRandomLed(int* ans)
{
    for (int i = 0; i < 3; ++i) {
        int led = LED[ans[i]];
        PrintPinSec(led, 1);
    }
    return;
}
static void GetInput()
{
    int repeat = 40;
    int idx = 0;
    visited[0] = visited[1] = visited[2] = -1;
    do {
        for (int i = 0; i < 3; ++i) {
            int button = Button[i];
            int result = GPIORead(button);
            if (0 == result && visited[i] == -1) {
                PrintPinSec(LED[i], 0.5);
                visited[i] = 1;
                guess[idx++] = i;
                printf("User Answer : %d order : %d\n", i, idx);
            }
        }
        usleep(100000);
    } while (repeat--);
    return;
}
static int isAnswer(int* ans, int* visited)
{
    int ret = 1;
    for (int i = 0; i < 3; ++i) {
        if (visited[i] != ans[i]) {
            return 0;
        }
    }
    return 1;
}
static void PrintResult(int isCorrect)
{
    int repeat = 20;
    int led = isCorrect ? SUCCESS : FAIL;

    for (int i = 0; i < 3; ++i) {
        PrintPinSec(led, 0.5f);
    }
}

int main(int argc, char* argv[])
{
    int repeat = 10;
    int state = 1;
    int prev_state = 1;
    int light = 0;
    //Enable GPIO pins
    //Enable LED, Button pins
    for (int i = 0; i < 3; ++i) {
        if (-1 == GPIOExport(LED[i]) || -1 == GPIOExport(Button[i])) {
            return(1);
        }
    }
    if (-1 == GPIOExport(SUCCESS) || -1 == GPIOExport(FAIL)) {
        return(1);
    }

    //Set GPIO directions
    //Set LED, Button directions
    for (int i = 0; i < 3; ++i) {
        int led = LED[i], button = Button[i];
        if (-1 == GPIODirection(led, OUT) || -1 == GPIODirection(button, IN)) {
            return(2);
        }
    }
    //Set SUCCESS, FAIL directions
    if (-1 == GPIODirection(SUCCESS, OUT) || -1 == GPIODirection(FAIL, OUT)) {
        return(2);
    }

    do {
        srand(time(NULL));
        int random = rand() % 6;
        int* ans = order[random];
        int correct = 0;
        printf("random : %d order : %d %d %d\n", random, ans[0], ans[1], ans[2]);

        PrintRandomLed(ans);
        GetInput();
        correct = isAnswer(ans, guess);
        PrintResult(correct);
        usleep(1000000);

    } while (repeat--);

    //Disable GPIO pins
    //Disable LED, Button pins
    for (int i = 0; i < 3; ++i) {
        if (-1 == GPIOUnexport(LED[i]) || -1 == GPIOUnexport(Button[i])) {
            return(4);
        }
    }
    //Diable SUCCESS, FAIL pins
    if (-1 == GPIOUnexport(SUCCESS) || -1 == GPIOUnexport(FAIL)) {
        return(4);
    }
    return(0);
}

static int GPIOExport(int pin)
{
#define BUFFER_MAX 3
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open export for writing!\n");
        return(-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return(0);
}
static int GPIODirection(int pin, int dir)
{
    static const char s_directions_str[] = "in\0out";
#define DIRECTION_MAX 35
    //char path[DIRECTION_MAX] = "/sys/class/gpio/gpio24/direction";
    char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio direction for writing!\n");
        return(-1);
    }

    if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
        fprintf(stderr, "Failed to set direction!\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(0);
}

static int GPIOWrite(int pin, int value)
{
    static const char s_values_str[] = "01";

    char path[VALUE_MAX];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return (-1);
    }

    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
        fprintf(stderr, "Failed to write value!\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(0);
}

static int GPIOUnexport(int pin)
{
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return (-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return(0);
}

static int GPIORead(int pin)
{
    char path[VALUE_MAX];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio value for reading!\n");
        return (-1);
    }

    if (-1 == read(fd, value_str, 3)) {
        fprintf(stderr, "Failed to read value!\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(atoi(value_str));
}