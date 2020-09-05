#include "poll_thread.h"
#include <thread>
#include <unistd.h>

using namespace Nan;

typedef struct
{
    Callback *callback;
    useconds_t delay;
    pthread_t thread;
    pthread_mutex_t mutex;
    uv_async_t async;
    bool isStopped = false;
} data_t;

data_t *data;

// void forwardCallback(uv_async_t *handle)
// {
//     Callback *callback = (Callback *)handle->data;
//     callback->Call(0, NULL);
// }

void *run(void *arg)
{
    data_t *data = (data_t *)arg;

    while (true)
    {
        pthread_mutex_lock(&data->mutex);
        bool isStopped = data->isStopped;
        pthread_mutex_unlock(&data->mutex);
        if (isStopped)
            return NULL;

        printf("poll\n");
        uv_async_send(&data->async);
        usleep(data->delay);
    }
}

void poll_start(Callback *callback, useconds_t delay)
{
    if (data)
        poll_stop();

    data = new data_t();
    data->callback = callback;
    data->delay = delay;
    data->async.data = data;

    uv_async_init(uv_default_loop(), &data->async, [](uv_async_t *handle) {
        EscapableHandleScope scope;
        data_t *data = (data_t *)handle->data;
        data->callback->Call(0, NULL);
    });

    pthread_create(&data->thread, NULL, run, data);
}

void poll_stop()
{
    pthread_mutex_lock(&data->mutex);
    data->isStopped = true;
    pthread_mutex_unlock(&data->mutex);
    pthread_join(data->thread, NULL);

    delete data->callback;
    uv_close((uv_handle_t *)&data->async, NULL);

    delete data;
    data = NULL;
}
