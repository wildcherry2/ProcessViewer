#include "ProcessController.h"
#include <thread>
#include <latch>
#include <map>
#include <set>
#include <chrono>
#include <atomic>
#include <vector>
#include <numeric>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include "include/base/cef_bind.h"
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/cef_task.h"
#include "include/wrapper/cef_helpers.h"
#include "Logger.h"
#include "LambdaV8Handler.h"
#include "LambdaTask.h"
#include "SearchEngine.h"
#include "platform/IProcessEventDispatcherImplPlatform.h"

extern std::shared_ptr<IProcessEventDispatcherImplPlatform> getPlatformImpl();

struct ProcessController::Impl {
    Impl() {
        platform->setCallback([this](const ProcessInfoUpdate& update) {
            update_queue.enqueue(update);
        });
    }

    void startMonitoring(CefRefPtr<CefV8Context> context) { //assumes context has been entered and this is being called in the render thread
        CEF_REQUIRE_RENDERER_THREAD();
        if(!context) {
            _RERR("[ProcessController] Can't start monitoring with a null context!");
            return;
        }
        if(!context->IsValid() || !context->InContext()) {
            _RERR("[ProcessController] Can't start monitoring out of context!");
            return;
        }
        if(platform->isListening()) {
            _RERR("[ProcessController] Can't start monitoring if monitoring has already started!");
            return;
        }
        ctx = context;
        auto global = context->GetGlobal();
        if(!global) {
            _RERR("[ProcessController] Can't start monitoring because the global object doesn't exist!");
            return;
        }
        if(!global->HasValue("NProcessController")) {
            auto obj_binding = CefV8Value::CreateObject(nullptr, nullptr);
            cef_v8_propertyattribute_t flags = static_cast<cef_v8_propertyattribute_t>(V8_PROPERTY_ATTRIBUTE_READONLY | V8_PROPERTY_ATTRIBUTE_DONTDELETE);

            auto signalJSReadyBinding = CefRefPtr(new LambdaV8Handler(std::bind(&ProcessController::Impl::signalJSReady, this)));
            auto signalTableHasElementsBinding = CefRefPtr(new LambdaV8Handler(std::bind(&ProcessController::Impl::signalTableHasElements, this)));
            auto jsEndTaskBinding = CefRefPtr(new LambdaV8Handler(std::bind(&ProcessController::Impl::jsEndTask, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,std::placeholders::_4,std::placeholders::_5)));
            auto jsUpdateSearchEngineTextBinding = CefRefPtr(new LambdaV8Handler(std::bind(&ProcessController::Impl::jsUpdateSearchEngineText, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,std::placeholders::_4,std::placeholders::_5)));

            obj_binding->SetValue("signalJSReady", CefV8Value::CreateFunction("signalJSReady", signalJSReadyBinding), flags);
            obj_binding->SetValue("signalTableHasElements", CefV8Value::CreateFunction("signalTableHasElements", signalTableHasElementsBinding), flags);
            obj_binding->SetValue("endTask", CefV8Value::CreateFunction("endTask", jsEndTaskBinding), flags);
            obj_binding->SetValue("updateSearchEngineText", CefV8Value::CreateFunction("updateSearchEngineText", jsUpdateSearchEngineTextBinding), flags);
            
            global->SetValue("NProcessController", obj_binding, flags);
            _RDEBUG("[ProcessController] Bound native object NProcessController!");
        }
        else {
            _RWARN("[ProcessController] Native object appears to alreay be bound to the global object!");
        }
        
        platform->startListening();

        update_thread = std::jthread(update_callback);
    }

    void stopMonitoring() {
        CEF_REQUIRE_RENDERER_THREAD();
        _RDEBUG("[ProcessController] Attempting to stop...");
        if(platform->isListening()) {
            platform->stopListening();
        }
        if(update_thread.request_stop()) {
            update_thread.join();
        }/*
        ctx = nullptr;
        table = nullptr;
        update_fn = nullptr;*/
        _RDEBUG("[ProcessController] Stopped monitoring threads!");
    }

private:
    std::function<void(std::stop_token)> update_callback = [this](std::stop_token stop_token) {
        std::map<unsigned long, std::shared_ptr<ProcessInfo>> processes; // maybe change value type to pointer?
        std::map<unsigned long, std::shared_ptr<ProcessTitleUpdateData>> pending_title_updates;
        std::vector<ProcessInfoUpdate> batch;
        batch.reserve(300);

        batch = platform->getAllProcessesAsAddedEvent();
        for(const auto& proc : batch) { //todo centralize onAddProcess
            auto pi = std::reinterpret_pointer_cast<ProcessInfo>(proc.data);
            processes[static_cast<unsigned long>(pi->pid)] = pi;
            auto pending_title_update = pending_title_updates.extract(pi->pid);
            if(!pending_title_update.empty()) {
                pi->title = pending_title_update.mapped()->title;
            }
            search_engine.addProcess(pi);
        }

        // wait for latch to be triggered from js, indicating that js is ready (firstUpdated in table)
        js_latch.wait();
        
        // send to js
        sendUpdateToJS(std::move(batch));

        // when updateComplete, show window
        update_complete_latch.wait();

        // main loop, push updates to js
        while(!stop_token.stop_requested()) {
            const auto start = std::chrono::steady_clock::now();
            for(size_t count = 0; count < UPDATE_BATCH_SIZE; ++count) {
                ProcessInfoUpdate update;
                const auto updated = update_queue.try_dequeue(update);
                if(!updated) break;
                auto send_to_js = true;
                switch(update.event) {
                    case EProcessInfoEvent::UNKNOWN: {
                        continue;
                    }
                    case EProcessInfoEvent::PROCESS_ADDED: {
                        auto data = std::reinterpret_pointer_cast<ProcessInfo>(update.data);
                        processes.insert_or_assign(data->pid, data);
                        auto pending_title_update = pending_title_updates.extract(data->pid);
                        if(!pending_title_update.empty()) {
                            data->title = pending_title_update.mapped()->title;
                        }
                        search_engine.addProcess(data);
                        break;
                    }
                    case EProcessInfoEvent::PROCESS_REMOVED: {
                        auto data = std::reinterpret_pointer_cast<unsigned long>(update.data);
                        processes.erase(*data);
                        search_engine.removeProcess(*data); 
                        break;
                    }
                    case EProcessInfoEvent::PROCESS_TITLEUPDATE: {
                        auto data = std::reinterpret_pointer_cast<ProcessTitleUpdateData>(update.data);
                        auto process = processes.find(data->pid);
                        if(process == processes.end()) {
                            send_to_js = false;
                            auto pending_update = pending_title_updates.find(data->pid);
                            if(pending_update != pending_title_updates.end()) {
                                if(pending_update->second->time < data->time) {
                                    pending_title_updates.insert_or_assign(data->pid, data);
                                    // todo log
                                }
                            }
                            else {
                                pending_title_updates.insert_or_assign(data->pid, data);
                            }
                        }
                        else {
                            auto pending = pending_title_updates.extract(data->pid);
                            if(!pending.empty()) {
                                data = data->time > pending.mapped()->time ? data : pending.mapped();
                                update.data = std::reinterpret_pointer_cast<void>(data);
                            }
                            process->second->title = data->title;
                        }
                        break;
                    }
                    case EProcessInfoEvent::CONTROL_TERMINATE: {
                        send_to_js = false;
                        auto data = std::reinterpret_pointer_cast<ProcessInfo>(update.data);
                        auto known = processes.find(data->pid);
                        if(known == processes.end() || known->second->pid != data->pid || known->second->exe_name != data->exe_name) break; //todo log
                        if(!platform->killProcess(known->second->pid)) {
                            _RERR("[ProcessController] Failed to kill process with pid ", data->pid, " and name ", data->exe_name, "!");
                        }
                        break;
                    }
                    case EProcessInfoEvent::CONTROL_UPDATE_SEARCH: {
                        auto data = std::reinterpret_pointer_cast<std::wstring>(update.data);
                        _RDEBUGW(L"[ProcessController] Update search: ", *data);
                        auto result = search_engine.updateSearch(*data);

                        if(!result) {
                            send_to_js = false;
                            break;
                        }
                        update.data = nullptr;
                        if(result.flags & SearchEngineResultFlags::SHOW_ALL) {
                            update.event = EProcessInfoEvent::PROCESS_SHOWALL;
                            break;
                        }
                        if(result.flags & SearchEngineResultFlags::HIDE_ALL) {
                            update.event = EProcessInfoEvent::PROCESS_HIDEALL;
                            break;
                        }
                        if(result.flags & SearchEngineResultFlags::SHOW_SUBSET) {
                            send_to_js = false;
                            update.event = EProcessInfoEvent::PROCESS_UNHIDDEN;
                            update.data = std::reinterpret_pointer_cast<void>(result.to_show);
                            batch.emplace_back(std::move(update));
                        }
                        if(result.flags & SearchEngineResultFlags::HIDE_SUBSET) {
                            send_to_js = false;
                            update.event = EProcessInfoEvent::PROCESS_HIDDEN;
                            update.data = std::reinterpret_pointer_cast<void>(result.to_hide);
                            batch.emplace_back(std::move(update));
                        }
                        break;
                    }
                    default: {
                        break; //todo log
                    }
                }
                
                if(send_to_js) batch.emplace_back(std::move(update));
                if(std::chrono::steady_clock::now() - start >= UPDATE_MIN_INTERVAL) break;
            }
            if(!batch.empty()) {
                sendUpdateToJS(std::move(batch));
            }
            const auto time = std::chrono::steady_clock::now() - start;
            if(time >= UPDATE_MIN_INTERVAL) continue;
            std::this_thread::sleep_for(UPDATE_MIN_INTERVAL - time);
        }
    };

    void sendUpdateToJS(std::vector<ProcessInfoUpdate>&& batch) {
        CefPostTask(TID_RENDERER, base::BindOnce(&ProcessController::Impl::execUpdate, base::Unretained(this), std::move(batch)));
    }

    void execUpdate(std::vector<ProcessInfoUpdate>&& batch) {
        CEF_REQUIRE_RENDERER_THREAD();

        if(!ctx || !ctx->IsValid()) {
            _RERR("[ProcessController] Failed to send update to JS because the context is invalid/nullptr!");
            return;
        }
            
        if(!ctx->Enter()) {
            _RERR("[ProcessController] Failed to send update to JS because the context couldn't be entered!");
            return;
        }

        if(!table) {
            auto global = ctx->GetGlobal();
            if(!global) {
                _RERR("[ProcessController] Failed to send update to JS because the context couldn't get the global object!");
                return;
            }
            table = global->GetValue("PTable");
            if(!table || !table->IsValid()) {
                _RERR("[ProcessController] Failed to send update to JS because PTable doesn't exist in the global object!");
                return;
            }
            update_fn = table->GetValue("updateTable");
            if(!update_fn || !update_fn->IsValid() ||!update_fn->IsFunction()) {
                _RERR("[ProcessController] Failed to send update to JS because updateTable doesn't exist on PTable or updateTable is not a function!");
                return;
            }
        }

        CefV8ValueList args;

        for(auto& update : batch) {
            auto out = CefV8Value::CreateObject(nullptr, nullptr);
            out->SetValue("event", CefV8Value::CreateUInt(static_cast<uint32_t>(update.event)), V8_PROPERTY_ATTRIBUTE_NONE);
            switch(update.event) {
                case EProcessInfoEvent::PROCESS_ADDED: {
                    auto update_data = std::reinterpret_pointer_cast<ProcessInfo>(update.data);
                    auto js_data = CefV8Value::CreateObject(nullptr, nullptr);
                    js_data->SetValue("pid", CefV8Value::CreateUInt(update_data->pid), V8_PROPERTY_ATTRIBUTE_NONE);
                    js_data->SetValue("name", CefV8Value::CreateString(update_data->exe_name), V8_PROPERTY_ATTRIBUTE_NONE);
                    js_data->SetValue("hidden", CefV8Value::CreateBool(update_data->hidden), V8_PROPERTY_ATTRIBUTE_NONE);
                    js_data->SetValue("path", CefV8Value::CreateString(update_data->exe_path), V8_PROPERTY_ATTRIBUTE_NONE);
                    if(update_data->exe_icon_b64)
                        js_data->SetValue("image", CefV8Value::CreateString(*(update_data->exe_icon_b64)), V8_PROPERTY_ATTRIBUTE_NONE);
                    if(update_data->title)
                        js_data->SetValue("title", CefV8Value::CreateString(*(update_data->title)), V8_PROPERTY_ATTRIBUTE_NONE);
                    // set other fields from ProcessInfo as supported  
                    
                    out->SetValue("data", js_data, V8_PROPERTY_ATTRIBUTE_NONE);
                    break;
                }
                case EProcessInfoEvent::PROCESS_REMOVED: {
                    auto update_data = std::reinterpret_pointer_cast<unsigned long>(update.data);
                    out->SetValue("data", CefV8Value::CreateUInt(*update_data), V8_PROPERTY_ATTRIBUTE_NONE);
                    break;
                }
                case EProcessInfoEvent::PROCESS_HIDEALL:
                case EProcessInfoEvent::PROCESS_SHOWALL: {
                    out->SetValue("data", CefV8Value::CreateUndefined(), V8_PROPERTY_ATTRIBUTE_NONE);
                    break;
                }
                case EProcessInfoEvent::PROCESS_UNHIDDEN:
                case EProcessInfoEvent::PROCESS_HIDDEN: {
                    auto update_data = std::reinterpret_pointer_cast<std::vector<unsigned long>>(update.data);
                    auto pid_array = CefV8Value::CreateArray(update_data->size());
                    CefV8ValueList pids;
                    pids.resize(update_data->size());
                    std::transform(update_data->begin(), update_data->end(), pids.begin(), CefV8Value::CreateUInt);
                    pid_array->GetValue("push")->ExecuteFunction(pid_array, pids);
                    out->SetValue("data", pid_array, V8_PROPERTY_ATTRIBUTE_NONE);
                    break;
                }
                case EProcessInfoEvent::PROCESS_TITLEUPDATE: {
                    auto update_data = std::reinterpret_pointer_cast<ProcessTitleUpdateData>(update.data);
                    auto js_data = CefV8Value::CreateObject(nullptr, nullptr);
                    js_data->SetValue("pid", CefV8Value::CreateUInt(update_data->pid), V8_PROPERTY_ATTRIBUTE_NONE);
                    js_data->SetValue("title", CefV8Value::CreateString(update_data->title ? *(update_data->title) : L""), V8_PROPERTY_ATTRIBUTE_NONE);
                    out->SetValue("data", js_data, V8_PROPERTY_ATTRIBUTE_NONE);
                    break;
                }
                default: {
                    _RERR("[ProcessController] Unsupported event attempted to pass to js: ", update.event);
                    continue;
                }
            }
            args.push_back(out);
        }

        if(!args.empty()) {
            update_fn->ExecuteFunction(table, args);
            _RDEBUG("[ProcessController] Pushed ", args.size(), " updates to JS!");
        } 
        else {
            _RERR("[ProcessController] Failed to push batch of size ", batch.size(), " to JS!");
        }

        if(!ctx->Exit()) {
            _RERR("[ProcessController] Failed to exit the context!");
        }
    }

    bool signalJSReady() {
        js_latch.count_down();
        return true;
    }

    bool signalTableHasElements() {
        update_complete_latch.count_down();
        return true;
    }

    // endTask(pid: number, exe_name: string): void 
    bool jsEndTask(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& args, CefRefPtr<CefV8Value>& retval, CefString& exception) {
        if(args.size() < 2) {
            exception = L"endTask: Not enough args!";
            return true;
        }
        auto pid = args[0];
        auto exe_name = args[1];
        if(!pid->IsUInt()) {
            exception = L"endTask: PID is not a UInt!";
            return true;
        }
        if(!exe_name->IsString()) {
            exception = L"endTask: exe_name is not a string!";
            return true;
        }
        // dont hold up render thread while validating and killing
        update_queue.enqueue({ EProcessInfoEvent::CONTROL_TERMINATE,  // might make js send file path
                                    std::reinterpret_pointer_cast<void>(std::make_shared<ProcessInfo>(exe_name->GetStringValue().ToWString(), L"", pid->GetUIntValue())) 
                             });
        return true;
    }

    // updateSearchEngineText(text: string): void
    bool jsUpdateSearchEngineText(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& args, CefRefPtr<CefV8Value>& retval, CefString& exception) {
        if(args.size() < 1) {
            exception = L"updateSearchEngineText: Not enough args!";
            return true;
        }
        auto text = args[0];
        if(!text->IsString()) {
            exception = L"updateSearchEngineText: text is not a string!";
            return true;
        }
        update_queue.enqueue({ EProcessInfoEvent::CONTROL_UPDATE_SEARCH, std::reinterpret_pointer_cast<void>(std::make_shared<std::wstring>(text->GetStringValue().ToWString())) });
        return true;
    }

    std::shared_ptr<IProcessEventDispatcherImplPlatform> platform = getPlatformImpl();
    moodycamel::ConcurrentQueue<ProcessInfoUpdate> update_queue = moodycamel::ConcurrentQueue<ProcessInfoUpdate>(250);
    std::jthread update_thread;
    std::latch js_latch = std::latch(1);
    std::latch update_complete_latch = std::latch(1);
    SearchEngine search_engine;
    //IconCache icon_cache = IconCache(getPlatformImpl());
    CefRefPtr<CefV8Context> ctx;
    CefRefPtr<CefV8Value> table;
    CefRefPtr<CefV8Value> update_fn;

    constexpr static auto UPDATE_MIN_INTERVAL = std::chrono::milliseconds(10); // will probably reduce
    constexpr static size_t UPDATE_BATCH_SIZE = 50;
};

ProcessController::ProcessController() : pimpl(std::make_unique<Impl>()) {}

void ProcessController::startMonitoring(CefRefPtr<CefV8Context> context) { return pimpl->startMonitoring(context); }

void ProcessController::stopMonitoring() { return pimpl->stopMonitoring(); }

ProcessController::~ProcessController() { pimpl->stopMonitoring(); }

std::shared_ptr<ProcessController> ProcessController::instance = nullptr;

std::shared_ptr<ProcessController> ProcessController::getInstance() {
    if(!instance) {
        instance = std::shared_ptr<ProcessController>(new ProcessController());
    }
    return instance;
}
