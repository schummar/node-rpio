#include <nan.h>
#include <node.h>
#include <unistd.h>
#include "watch.h"

using namespace v8;
using std::string;

class Work : public Nan::AsyncWorker
{
public:
    Work(Nan::Callback *callback) : AsyncWorker(callback){};
    ~Work(){};
    void Execute();
    void Destory(){};

private:
    string data = "";

protected:
    void HandleOKCallback();
    void HandleErrorCallback(){};
};

void Work::Execute()
{
    this->data = "done";
    sleep(3);
}

void Work::HandleOKCallback()
{
    Nan::HandleScope scope;
    Local<Value> argv[] = {Nan::New<String>(this->data).ToLocalChecked()};
    this->callback->Call(1, argv);
}

void watch(uint32_t pin, Nan::Callback *callback)
{
    Nan::AsyncQueueWorker(new Work(callback));
}

void unwatch(uint32_t pin) {}
