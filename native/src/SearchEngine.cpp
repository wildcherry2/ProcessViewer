#include "SearchEngine.h"
#include <algorithm>
#include <ranges>
#include <cwctype>
#include "Logger.h"

std::wstring toLowerCase(const std::wstring& string) {
    std::wstring lower;
    lower.resize(string.size());
    std::transform(string.begin(), string.end(), lower.begin(), std::towlower);
    return lower;
}

void SearchEngine::addProcess(std::shared_ptr<ProcessInfo> process) {
    process->hidden = shouldHideProcess(process, current_filters);
    auto& map = process->hidden ? hidden : visible;
    map[process->pid] = process;
}

void SearchEngine::removeProcess(const unsigned long pid) {
    auto* target_map = visible.size() > hidden.size() ? &visible : &hidden;
    if(!target_map->erase(pid)) {
        target_map = target_map == &visible ? &hidden : &visible;
        target_map->erase(pid);
    }
}

SearchEngineUpdateResult SearchEngine::updateSearch(const std::wstring& new_text) {
    if(new_text == current_filters.current_text) {
        _RDEBUG("[SearchEngine] updateSearch called with no changes!");
        return {};
    }

    auto old_text = std::move(current_filters.current_text); 
    current_filters = std::move(FilterGroup(new_text, regex));

    if(current_filters.ungrouped.size() > 1) {
        _RDEBUG("[SearchEngine] Hiding all processes because of multiple ungrouped strings!");
        // todo update maps
        hidden.merge(visible); 
        if(!visible.empty()) {
            _RERR("[SearchEngine] Merging processes into hidden didn't capture all processes because there were duplicates in visible!");
        }
        return { SearchEngineResultFlags::HIDE_ALL };
    }

    if(isTransitionToShowAll(old_text)) {
        _RDEBUG("[SearchEngine] Transitioning to showing all processes!");
        // todo update maps
        visible.merge(hidden);
        if(!hidden.empty()) {
            _RERR("[SearchEngine] Merging processes into hidden didn't capture all processes because there were duplicates in hidden!");
        }
        return { SearchEngineResultFlags::SHOW_ALL };
    }

    auto result = SearchEngineUpdateResult{};

    auto visible_bigger = visible.size() > hidden.size();
    visible_bigger ? processVector(hidden, visible, &SearchEngine::shouldShowProcess,
                                   result.to_show, result.flags, SHOW_SUBSET, SHOW_ALL, current_filters) :
        processVector(visible, hidden, &SearchEngine::shouldHideProcess,
                      result.to_hide, result.flags, HIDE_SUBSET, HIDE_ALL, current_filters);
    visible_bigger ? processVector(visible, hidden, &SearchEngine::shouldHideProcess,
                                   result.to_hide, result.flags, HIDE_SUBSET, HIDE_ALL, current_filters) :
        processVector(hidden, visible, &SearchEngine::shouldShowProcess,
                      result.to_show, result.flags, SHOW_SUBSET, SHOW_ALL, current_filters);


    return result;
}

bool SearchEngine::shouldHideProcess(std::shared_ptr<ProcessInfo> process, const FilterGroup& filters) {
    return !shouldShowProcess(process, filters);
}

bool SearchEngine::shouldShowProcess(std::shared_ptr<ProcessInfo> process, const FilterGroup& filters) {
    if(filters.ungrouped.size() > 1) return false;
    if(filters.allowed_pids.size()) {
        // must pass some 'pid' filter
        if(std::ranges::none_of(filters.allowed_pids, [&process](const unsigned long pid){ return process->pid == pid; })) {
            _RDEBUGW(L"[SearchEngine] Hiding process (PID filter): ", process->pid, L" ", process->exe_name, L"!");
            return false;
        }
    }
    const auto exe_lower = toLowerCase(process->exe_name); // todo maybe cache in ProcessInfo
    const auto title_lower = process->title ? toLowerCase(*process->title) : L"";
    if(filters.contains.size()) {
        // must pass every 'contains' filter to remain visible
        if(!std::ranges::all_of(filters.contains, [&exe_lower, &title_lower](const std::wstring& contains_substr){ 
            return (exe_lower.find(contains_substr) != std::wstring::npos) || (title_lower.find(contains_substr) != std::wstring::npos);
        })) {
            _RDEBUGW(L"[SearchEngine] Hiding process (contains filter): ", process->pid, L" ", process->exe_name, L"!");
            return false;
        }
    }
    if(filters.title.size()) {
        // must pass some 'title' filter
        if(!std::ranges::any_of(filters.title, [&title_lower](const std::wstring& title_substr){ return title_lower.starts_with(title_substr); })) {
            _RDEBUGW(L"[SearchEngine] Hiding process (title filter): ", process->pid, L" ", process->exe_name, L"!");
            return false;
        }
    }
    if(filters.ungrouped.size()) {
        // if there's an ungrouped filter, it has to pass a starts_with filter
        if(!exe_lower.starts_with(filters.ungrouped[0]) && !title_lower.starts_with(filters.ungrouped[0])) {
            _RDEBUGW(L"[SearchEngine] Hiding process (starts_with filter): ", process->pid, L" ", process->exe_name, L"!");
            return false;
        }
    }
    return true;
}

bool SearchEngine::isTransitionToShowAll(const std::wstring& old_text) {
    return current_filters.current_text.empty() && !old_text.empty();
}

// should also change process.hidden as needed
void SearchEngine::swapMaps(SEProcessMap::iterator it_pair, SEProcessMap& src, SEProcessMap& dest) {
    auto process = it_pair->second; // might be able to std::move this (TODO use extract)
    src.erase(it_pair);
    auto inserted = dest.insert_or_assign(process->pid, process);
    inserted.first->second->hidden = &dest == &hidden;
}

void SearchEngine::processVector(SEProcessMap& search_vec,
                                 SEProcessMap& swap_vec,
                                 bool(SearchEngine::* predicate)(std::shared_ptr<ProcessInfo>, const FilterGroup& filters),
                                 std::shared_ptr<std::vector<unsigned long>>& result_vec,
                                 unsigned int& flag,
                                 unsigned int subset_flag,
                                 unsigned int all_flag, const FilterGroup& filters) {
    if(search_vec.empty()) {
        flag |= SearchEngineResultFlags::NONE;
        _RDEBUG("[SearchEngine] Empty vector, skipping processing!");
        return;
    }
    
    for(auto pair = search_vec.begin(); pair != search_vec.end();) {
        if((this->*predicate)(pair->second, filters)) {
            auto pid = pair->first;
            auto erase = pair++;
            swapMaps(erase, search_vec, swap_vec);
            if(!result_vec) {
                result_vec = std::make_shared<std::vector<unsigned long>>();
                flag = subset_flag;
            }
            result_vec->push_back(pid);
        }
        else {
            ++pair;
        }
    }
    if(search_vec.empty()) flag = all_flag;
}

FilterGroup::FilterGroup(const std::wstring& text, const std::wregex& regex)  {
    for(std::wsregex_iterator it(text.begin(), text.end(), regex), end; it != end; ++it) {
        const auto& m = *it;
        try {
            if(m[1].matched) { 
                allowed_pids.push_back(std::stoi(m[1]));
                _RDEBUGW(L"[SearchEngine] Added pid ", m[1], L" to filters!");
            }
            else if(m[3].matched) {
                title.push_back(toLowerCase(m[3]));
                _RDEBUGW(L"[SearchEngine] Added title string \"", m[3], L"\" to filters!");
            }
            else if(m[2].matched) { 
                contains.push_back(toLowerCase(m[2])); 
                _RDEBUGW(L"[SearchEngine] Added contains string \"", m[2], L"\" to filters!");
            }
            else if(m[4].matched) { 
                ungrouped.push_back(toLowerCase(m[4]));
                _RDEBUGW(L"[SearchEngine] Added starts_with string \"", m[4], L"\" to filters!");
            }
        } 
        catch(const std::exception& ex) {
            _RERR("[SearchEngine] Error parsing search text: ", ex.what(), "! (", m[1].matched, "; ", m[2].matched, "; ", m[3].matched, ")");
            continue;
        }
    }
    current_text = text;
}
