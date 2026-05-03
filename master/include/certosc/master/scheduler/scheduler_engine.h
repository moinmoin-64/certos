#pragma once

#include "certosc/common/types.h"
#include "certosc/master.pb.h"
#include "certosc/master/job_manager.h"
#include "certosc/master/node_registry.h"
#include "certosc/master/scheduler/fair_share.h"
#include <thread>
#include <atomic>
#include <memory>

namespace certosc {

class SchedulerEngine {
public:
    SchedulerEngine(std::shared_ptr<JobManager> job_mgr, std::shared_ptr<NodeRegistry> node_registry, std::shared_ptr<FairShareManager> fair_share);
    ~SchedulerEngine();

    void start(uint32_t interval_ms);
    void stop();

private:
    void scheduling_loop();
    void schedule_pass();

    std::shared_ptr<JobManager> job_mgr_;
    std::shared_ptr<NodeRegistry> node_registry_;
    std::shared_ptr<FairShareManager> fair_share_;
    
    std::atomic<bool> running_{false};
    uint32_t interval_ms_;
    std::thread thread_;
};

} // namespace certosc
