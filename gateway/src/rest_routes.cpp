#include "certosc/gateway/rest_routes.h"
#include "certosc/common/logging.h"
#include <fstream>
#include <filesystem>

namespace certosc {

namespace fs = std::filesystem;

void RestRoutes::add_cors_headers(http::response<http::string_body>& res, const std::string& origin) {
    res.set(http::field::access_control_allow_origin, origin);
    res.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
    res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
}

http::response<http::string_body> RestRoutes::handle(
    const http::request<http::string_body>& req, 
    std::shared_ptr<GrpcBridge> bridge,
    const GatewayConfig& config) 
{
    std::string path = std::string(req.target());
    LOG_DEBUG("HTTP: {} {}", req.method_string(), path);
    
    // ─── Static File Serving ────────────────────────────────────────────────
    if (path.find("/api/") != 0) {
        if (path == "/favicon.ico") {
            http::response<http::string_body> res{http::status::no_content, req.version()};
            res.prepare_payload();
            return res;
        }
        
        if (path == "/" || path == "/index.html") path = "/login.html";
        
        fs::path full_path = fs::path(config.web_root) / path.substr(1);
        if (fs::exists(full_path) && !fs::is_directory(full_path)) {
            std::ifstream file(full_path, std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.body() = std::move(content);
            
            // Basic Content-Type detection
            std::string ext = full_path.extension().string();
            if (ext == ".html") res.set(http::field::content_type, "text/html");
            else if (ext == ".css") res.set(http::field::content_type, "text/css");
            else if (ext == ".js") res.set(http::field::content_type, "application/javascript");
            else if (ext == ".png") res.set(http::field::content_type, "image/png");
            else res.set(http::field::content_type, "application/octet-stream");
            
            res.prepare_payload();
            return res;
        }
        
        // Fallback for SPA routing or just 404
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.body() = "404 Not Found";
        res.prepare_payload();
        return res;
    }

    // ─── API Routes ─────────────────────────────────────────────────────────
    http::response<http::string_body> res{http::status::ok, req.version()};
    add_cors_headers(res, config.cors_origin);
    res.set(http::field::server, "CertOS Gateway");
    res.set(http::field::content_type, "application/json");

    if (req.method() == http::verb::options) {
        res.prepare_payload();
        return res;
    }
    
    // Extract Bearer token
    std::string token;
    auto auth_it = req.find(http::field::authorization);
    if (auth_it != req.end()) {
        std::string val = std::string(auth_it->value());
        if (val.find("Bearer ") == 0) {
            token = val.substr(7);
        }
    }

    boost::json::object response_json;

    try {
        LOG_INFO("API Request: {} {}", req.method_string(), path);

        // Normalize path: remove trailing slash if exists
        std::string norm_path = path;
        if (norm_path.length() > 1 && norm_path.back() == '/') {
            norm_path.pop_back();
        }

        if (req.method() == http::verb::post && norm_path == "/api/v1/auth/login") {
            auto body = boost::json::parse(req.body()).as_object();
            response_json = bridge->login(body);
        }
        else if (req.method() == http::verb::post && norm_path == "/api/v1/public/request") {
            auto body = boost::json::parse(req.body()).as_object();
            JobRequest pb_req;
            pb_req.set_applicant_name(std::string(body["name"].as_string()));
            pb_req.set_applicant_email(std::string(body["email"].as_string()));
            pb_req.set_project_goal(std::string(body["goal"].as_string()));
            
            auto* spec = pb_req.mutable_requested_spec();
            spec->set_name("Public Request: " + pb_req.applicant_name());
            auto* res_req = spec->mutable_resources();
            res_req->set_cpu_cores(static_cast<int32_t>(body["cpu"].as_int64()));
            res_req->set_ram_mb(static_cast<int32_t>(body["ram"].as_int64() * 1024));
            spec->set_time_limit_seconds(static_cast<int32_t>(body["time"].as_int64() * 3600));
            
            if (body.contains("container")) {
                spec->set_container_image(std::string(body["container"].as_string()));
            }
            
            response_json = bridge->submit_public_request(pb_req);
        }
        else if (req.method() == http::verb::post && norm_path.find("/api/v1/public/request/") == 0 && norm_path.find("/upload") != std::string::npos) {
            size_t start = 22; // length of /api/v1/public/request/
            size_t end = norm_path.find("/upload");
            std::string req_id = norm_path.substr(start, end - start);
            
            std::string upload_dir = "./uploads";
            fs::create_directories(upload_dir);
            std::string file_path = upload_dir + "/" + req_id + ".tar.gz";
            
            std::ofstream ofs(file_path, std::ios::binary);
            if (!ofs) {
                LOG_ERROR("Could not open file for writing: {}", file_path);
                throw std::runtime_error("Could not open file for writing");
            }
            ofs.write(req.body().data(), req.body().size());
            ofs.close();
            
            // Link archive to the request in Master DB
            auto update_res = bridge->update_request_archive(req_id, file_path);
            
            response_json["success"] = update_res["success"];
            if (!update_res["success"].as_bool()) {
                response_json["error"] = update_res["error"];
            } else {
                response_json["message"] = "File uploaded and linked successfully";
            }
            LOG_INFO("Uploaded archive for request {}: {}", req_id, file_path);
        }
        else if (req.method() == http::verb::get && norm_path == "/api/v1/cluster") {
            response_json = bridge->get_cluster_status(token);
        }
        else if (req.method() == http::verb::get && norm_path == "/api/v1/nodes") {
            response_json = bridge->list_nodes(token);
        }
        else if (req.method() == http::verb::get && norm_path == "/api/v1/jobs") {
            response_json = bridge->list_jobs(token);
        }
        else if (req.method() == http::verb::post && norm_path == "/api/v1/jobs") {
            auto body = boost::json::parse(req.body()).as_object();
            response_json = bridge->submit_job(token, body);
        }
        else if (req.method() == http::verb::delete_ && norm_path.find("/api/v1/jobs/") == 0) {
            std::string job_id = norm_path.substr(13);
            response_json = bridge->cancel_job(token, job_id);
        }
        else if (req.method() == http::verb::get && norm_path == "/api/v1/admin/requests") {
            response_json = bridge->admin_list_requests(token);
        }
        else if (req.method() == http::verb::post && norm_path.find("/api/v1/admin/requests/") == 0 && norm_path.find("/review") != std::string::npos) {
            size_t start = 24;
            size_t end = norm_path.find("/review");
            std::string req_id = norm_path.substr(start, end - start);
            LOG_INFO("Admin Review Request: ID={}, Path={}", req_id, norm_path);
            auto body = boost::json::parse(req.body()).as_object();
            response_json = bridge->admin_review_request(token, req_id, body["approve"].as_bool(), std::string(body["comment"].as_string()));
        }
        else if (req.method() == http::verb::get && norm_path == "/api/v1/admin/users") {
            response_json = bridge->admin_list_users(token);
        }
        else if (req.method() == http::verb::post && norm_path == "/api/v1/admin/users") {
            auto body = boost::json::parse(req.body()).as_object();
            response_json = bridge->create_user(token, body);
        }
        else if (req.method() == http::verb::delete_ && norm_path.find("/api/v1/admin/users/") == 0) {
            std::string uname = norm_path.substr(20);
            response_json = bridge->admin_delete_user(token, uname);
        }
        else {
            res.result(http::status::not_found);
            response_json["success"] = false;
            response_json["error"] = "Route not found: " + norm_path;
            LOG_WARN("Route not found: {}", norm_path);
        }
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        response_json["success"] = false;
        response_json["error"] = std::string("Internal error: ") + e.what();
        LOG_ERROR("API Exception: {}", e.what());
    }

    std::string res_body = boost::json::serialize(response_json);
    LOG_DEBUG("API Response: {}", res_body);
    res.body() = std::move(res_body);
    res.prepare_payload();
    return res;
}

} // namespace certosc
