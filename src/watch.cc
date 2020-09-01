#include <nan.h>
#include <node.h>
#include <unistd.h>
#include <uv.h>
#include <poll.h>
#include "bcm2835.h"
#include "watch.h"

using namespace v8;
using namespace Nan;
using namespace std;

#define MAX_GPIO 40

typedef struct
{
    Callback *callback;
    uint32_t direction;
} config;

pthread_mutex_t mutex;
config configs[MAX_GPIO];
pthread_t thread;
int pipe_down[2];
int pipe_up[2];
uv_async_t handle;

void *poll_thread(void *vargp)
{
    bool needsUpdate = true;
    pollfd descriptors[MAX_GPIO + 1];
    descriptors[MAX_GPIO].fd = pipe_down[1];
    descriptors[MAX_GPIO].events = POLLIN;

    for (;;)
    {
        if (needsUpdate)
        {
            pthread_mutex_lock(&mutex);

            for (int i = 0; i < MAX_GPIO; i++)
            {
                if (configs[i].callback && !descriptors[i].fd)
                {
                    char fileName[80];
                    sprintf(fileName, "/sys/class/gpio/gpio%s/value", i);
                    descriptors[i].fd = open(fileName, O_RDONLY);
                    descriptors[i].events = POLLPRI;
                }
                else if (!configs[i].callback && descriptors[i].fd)
                {
                    close(descriptors[i].fd);
                    descriptors[i].fd = NULL;
                }
            }

            needsUpdate = false;

            pthread_mutex_unlock(&mutex);
        }

        poll(descriptors, MAX_GPIO + 1, -1);

        for (int i = 0; i < MAX_GPIO; i++)
        {
            if (descriptors[i].revents & POLLPRI)
            {
                printf("event %s\n", i);
                write(pipe_up[0], &i, sizeof(i));
            }
        }
        if (descriptors[MAX_GPIO].revents & POLLIN)
        {
            printf("need update\n");
            needsUpdate = true;
        }
    }
}

void handleCallback(uv_async_t *handle)
{
    uint32_t pin;
    while (read(pipe_up[1], &pin, sizeof(pin)))
    {
        if (configs[pin].callback)
        {
            Local<Value> arg = Nan::New(pin);
            configs[pin].callback->Call(1, &arg);
        }
    }
}

void watch(uint32_t gpio, Callback *callback, uint32_t direction)
{
    pthread_mutex_lock(&mutex);
    configs[gpio].callback = callback;
    configs[gpio].direction = direction;
    pthread_mutex_unlock(&mutex);

    if (!thread)
    {
        pipe(pipe_down);
        pipe(pipe_up);
        uv_async_init(uv_default_loop(), &handle, handleCallback);
        pthread_create(&thread, NULL, poll_thread, NULL);
    }
    else
    {
        write(pipe_down[0], "1", 1);
    }
}

void unwatch(uint32_t pin)
{
    pthread_mutex_lock(&mutex);
    delete configs[pin].callback;
    configs[pin].callback = NULL;
    pthread_mutex_unlock(&mutex);
}
