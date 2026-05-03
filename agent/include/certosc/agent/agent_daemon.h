#pragma once

#include "certosc/common/config.h"
#include "certosc/common/types.h"
#include "certosc/agent/agent_grpc_client.h"
#include "certosc/agent/job_executor.h"
#include "certosc/metrics/system_monitor.h"

#include <memory>
#include <thread>
#include <atomic>
#include <string>

namespace certosc {

class AgentDaemon {
public:
    explicit AgentDaemon(const AgentConfig& config);
    ~AgentDaemon();

    void start();
    void stop();

private:
    void heartbeat_loop();
    void job_polling_loop();
    
    AgentConfig config_;
    std::string node_id_;
    std::atomic<bool> running_{false};
    
    std::shared_ptr<AgentGrpcClient> grpc_client_;
    std::shared_ptr<JobExecutor> executor_;
    std::shared_ptr<SystemMonitor> monitor_;
    
    std::thread heartbeat_thread_;
    std::thread polling_thread_;
};

} // namespace certosc
