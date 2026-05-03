#pragma once

#include "certosc/common/config.h"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <mutex>
#include <optional>

namespace certosc {

class StateStore {
public:
    explicit StateStore(const std::string& db_path);
    ~StateStore();

    bool init();

    // -- Key-Value store (for simple configs/states) --
    bool set_kv(const std::string& key, const std::string& value);
    std::optional<std::string> get_kv(const std::string& key);

    // -- Job Requests --
    bool add_job_request(const std::string& req_id, const std::string& email, int status, const std::string& data);
    bool update_request_status(const std::string& req_id, int status, const std::string& data);
    std::vector<std::pair<std::string, std::string>> list_requests(); // Returns id and data json

private:
    std::string db_path_;
    sqlite3* db_ = nullptr;
    std::mutex mutex_;

    bool execute(const std::string& sql);
};

} // namespace certosc
