#include <nan.h>
#include <node.h>
#include <unistd.h>
#include <uv.h>
#include <poll.h>
#include "bcm2835.h"
#include "watch.h"

using namespace v8;
using namespace Nan;
using std::string;

typedef struct
{
    Callback *callback;
    bool direction;
} watcher;

pthread_mutex_t mutex;
watcher watchers[40];
bool poll_thread_running = false;
int pipe_fds[2];

void *poll_thread(void *vargp)
{
    bool isInit = false;
    pollfd descriptors[40];
    nfds_t nfds;

    for (;;)
    {
        if (!isInit)
        {
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < 40; i++)
            {
                if (watchers[i].callback)
                {
                    descriptors[i].fd = open(i);
                }
                else
                {
                    if (descriptors[i].fd)
                    {
                        close(descriptors[i].fd);
                    }
                    descriptors[i].fd = NULL;
                }
            }
            pthread_mutex_unlock(&mutex);

            poll(descriptors, &nfds);
        }
    }
}

void watch(uint32_t pin, Callback *callback)
{
    pthread_mutex_lock(&mutex);

    watchers[pin].callback = callback;

    if (!poll_thread_running)
    {
        poll_thread_running = true;
        pipe(pipe_fds);
        pthread_t thread;
        pthread_create(&thread, NULL, poll_thread, NULL);
    }
    else
    {
        char msg = 1;
        write(pipe_fds[0], &msg, 1);
    }

    pthread_mutex_unlock(&mutex);
}

void unwatch(uint32_t pin)
{
    pthread_mutex_lock(&mutex);
    delete watchers[pin].callback;
    watchers[pin].callback = NULL;
    pthread_mutex_unlock(&mutex);
}
