#include "LambdaTask.h"

LambdaTask::LambdaTask(TaskCallback cb) : cb(cb) {}

void LambdaTask::Execute() { 
    return cb(); 
}
