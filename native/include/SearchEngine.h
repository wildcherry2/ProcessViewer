#pragma once
#include <map>
#include <vector>
#include <memory>
#include <regex>
#include <string_view>
#include "ProcessInfo.h"

enum SearchEngineResultFlags {
    NONE = 0,           // no change
    SHOW_SUBSET = 1,    // show some amount of processes (to_show != nullptr)    
    HIDE_SUBSET = 2,    // hide some amount of processes (to_hide != nullptr)
    SHOW_ALL = 4,       // show all processes
    HIDE_ALL = 8        // hide all processes
};

struct SearchEngineUpdateResult {
    unsigned int flags = SearchEngineResultFlags::NONE;
    std::shared_ptr<std::vector<unsigned long>> to_show;
    std::shared_ptr<std::vector<unsigned long>> to_hide;
    explicit operator bool() const { return flags; }
};

struct FilterGroup {
    FilterGroup() = default;
    FilterGroup(const std::wstring& text, const std::wregex& regex); // might save some allocations by making this reusable

    std::vector<unsigned long> allowed_pids;
    std::vector<std::wstring> contains;
    std::vector<std::wstring> title;
    std::vector<std::wstring> ungrouped;
    std::wstring current_text;
};

class SearchEngine {
public:
    void addProcess(std::shared_ptr<ProcessInfo> process);
    void removeProcess(const unsigned long pid);
    SearchEngineUpdateResult updateSearch(const std::wstring& new_text);
private:
    using SEProcessMap = std::map<unsigned long, std::shared_ptr<ProcessInfo>>;

    bool shouldHideProcess(std::shared_ptr<ProcessInfo> process, const FilterGroup& filters);
    bool shouldShowProcess(std::shared_ptr<ProcessInfo> process, const FilterGroup& filters);
    bool isTransitionToShowAll(const std::wstring& new_text);
    void swapMaps(SEProcessMap::iterator it_pair, SEProcessMap& src, SEProcessMap& dest);

    void processVector(SEProcessMap& search_vec, 
                       SEProcessMap& swap_vec, 
                       bool (SearchEngine::*predicate)(std::shared_ptr<ProcessInfo>,const FilterGroup&), 
                       std::shared_ptr<std::vector<unsigned long>>& result_vec, 
                       unsigned int& flag,
                       unsigned int subset_flag, 
                       unsigned int all_flag, const FilterGroup& filters);

    SEProcessMap hidden;
    SEProcessMap visible;
    FilterGroup current_filters;
    std::wregex regex = std::wregex(L"pid:(\\d+)|contains:([^\\s]+)|title:([^\\s]+)|([^\\s]+)");
    //SearchEngineResultFlags last_flag = SearchEngineResultFlags::SHOW_ALL;
};