#pragma once
#include <functional>
#include "include/cef_task.h"

class LambdaTask : public CefTask {
public:
    using TaskCallback = std::function<void()>;

    LambdaTask(TaskCallback cb);
    LambdaTask() = delete;
    ~LambdaTask() override = default;

    void Execute() override;
private:
    IMPLEMENT_REFCOUNTING(LambdaTask);
    TaskCallback cb;
};