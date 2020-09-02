#include "poll_thread.h"
#include <thread>
#include <unistd.h>

using namespace Nan;

void forwardCallback(uv_async_t *handle)
{
    Callback *callback = (Callback *)handle->data;
    callback->Call(0, NULL);
}

class Worker
{
public:
    Worker(Callback *callback, useconds_t delay = 1000) : callback(callback), delay(delay) {}

    void *start()
    {
        uv_async_init(uv_default_loop(), &this->async, forwardCallback);
        this->async.data = this->callback;
        std::thread(Worker::run, this);
    }

    void stop()
    {
        pthread_mutex_lock(&this->mutex);
        this->isStopped = true;
        uv_close((uv_handle_t *)&this->async, NULL);
        delete this->callback;
        pthread_mutex_unlock(&this->mutex);
    }

private:
    Callback *callback;
    useconds_t delay;

    pthread_t thread;
    pthread_mutex_t mutex;
    uv_async_t async;
    bool isStopped = false;

    void *run(void *vargp)
    {
        while (true)
        {
            pthread_mutex_lock(&this->mutex);
            bool isStopped = this->isStopped;
            if (!isStopped)
            {
                printf("poll\n");
                uv_async_send(&this->async);
            }
            pthread_mutex_unlock(&this->mutex);

            if (isStopped)
                return NULL;

            usleep(this->delay);
        }
    }
};

Worker *worker;

void poll_start(Callback *callback, useconds_t delay)
{
    if (worker)
        poll_stop();

    worker = new Worker(callback, delay);
    worker->start();
}

void poll_stop()
{
    worker->stop();
    delete worker;
    worker = NULL;
}
