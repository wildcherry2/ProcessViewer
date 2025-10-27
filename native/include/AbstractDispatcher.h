#pragma once
#include <functional>
#include <map>
#include <thread>
#include <shared_mutex>
#include <stdexcept>

template<typename... Args>
class AbstractDispatcher {
public:
    using ListenerToken = uint64_t;
    using Callback = std::function<void(Args...)>;

    virtual void addListener(Callback callback, ListenerToken& out_token) {
        if(!callback) {
            throw std::invalid_argument("Callback cannot be null.");
        }

        std::unique_lock lock(mutex);
        out_token = token_counter++;
        callbacks[out_token] = callback;
    }

    virtual void removeListener(ListenerToken token) {
        std::unique_lock lock(mutex);
        callbacks.erase(token); //throws?
    }

    virtual ~AbstractDispatcher() = default;

protected:
    AbstractDispatcher() = default;

    bool hasListeners() {
        std::shared_lock lock(mutex);
        return !callbacks.empty();
    }

    virtual void notifyListeners(Args&&... args) {
        std::shared_lock lock(mutex);
        for(const auto& [token, callback] : callbacks) {
            callback(std::forward<Args>(args)...);
        }
    }

private:
    ListenerToken token_counter = 1;
    std::map<ListenerToken, Callback> callbacks;
    std::shared_mutex mutex;
};