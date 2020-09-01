#include <unistd.h>
#include <nan.h>

void watch(uint32_t pin, Nan::Callback *callback, uint32_t direction);
void unwatch(uint32_t pin);
