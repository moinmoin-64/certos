#include "certosc/master/state_store.h"
#include "certosc/common/logging.h"

namespace certosc {

StateStore::StateStore(const std::string& db_path) : db_path_(db_path) {}

StateStore::~StateStore() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool StateStore::init() {
    std::lock_guard<std::mutex> lock(mutex_);
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to open database {}: {}", db_path_, sqlite3_errmsg(db_));
        return false;
    }

    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS kv_store (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS jobs (
            job_id TEXT PRIMARY KEY,
            user_id TEXT,
            status INTEGER,
            data TEXT -- JSON representation
        );
        CREATE TABLE IF NOT EXISTS job_requests (
            request_id TEXT PRIMARY KEY,
            applicant_email TEXT,
            status INTEGER,
            data TEXT -- JSON representation
        );
    )";

    return execute(schema);
}

bool StateStore::add_job_request(const std::string& req_id, const std::string& email, int status, const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string sql = "INSERT INTO job_requests (request_id, applicant_email, status, data) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, req_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, status);
    sqlite3_bind_text(stmt, 4, data.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool StateStore::update_request_status(const std::string& req_id, int status, const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string sql = "UPDATE job_requests SET status = ?, data = ? WHERE request_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_text(stmt, 2, data.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, req_id.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<std::pair<std::string, std::string>> StateStore::list_requests() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, std::string>> results;
    std::string sql = "SELECT request_id, data FROM job_requests ORDER BY request_id DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            results.push_back({id, data});
        }
    }
    sqlite3_finalize(stmt);
    return results;
}

bool StateStore::execute(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL error: {}", err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool StateStore::set_kv(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string sql = "INSERT OR REPLACE INTO kv_store (key, value) VALUES ('" + key + "', '" + value + "');";
    return execute(sql);
}

std::optional<std::string> StateStore::get_kv(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string sql = "SELECT value FROM kv_store WHERE key = '" + key + "';";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    std::optional<std::string> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return result;
}

} // namespace certosc
