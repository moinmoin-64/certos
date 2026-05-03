#include "certosc/master/scheduler/scheduler_engine.h"
#include "certosc/common/logging.h"
#include <chrono>

namespace certosc {

SchedulerEngine::SchedulerEngine(std::shared_ptr<JobManager> job_mgr, std::shared_ptr<NodeRegistry> node_registry, std::shared_ptr<FairShareManager> fair_share)
    : job_mgr_(job_mgr), node_registry_(node_registry), fair_share_(fair_share) {
}

SchedulerEngine::~SchedulerEngine() {
    stop();
}

void SchedulerEngine::start(uint32_t interval_ms) {
    if (running_) return;
    interval_ms_ = interval_ms;
    running_ = true;
    thread_ = std::thread(&SchedulerEngine::scheduling_loop, this);
    LOG_INFO("Scheduler engine started (interval: {} ms)", interval_ms);
}

void SchedulerEngine::stop() {
    if (!running_) return;
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
    LOG_INFO("Scheduler engine stopped.");
}

void SchedulerEngine::scheduling_loop() {
    while (running_) {
        schedule_pass();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    }
}

void SchedulerEngine::schedule_pass() {
    auto pending_jobs = job_mgr_->get_pending_jobs();
    if (pending_jobs.empty()) {
        return; // Nothing to schedule
    }

    // Phase 3: Advanced Scheduling with Fair-Share Priority
    // Apply decay to fair-share metrics periodically (simplified here per pass)
    fair_share_->apply_decay();

    // Sort by effective priority (base * fair_share_factor) then by submission time
    std::sort(pending_jobs.begin(), pending_jobs.end(), [this](const JobInfo& a, const JobInfo& b) {
        double factor_a = fair_share_->get_priority_factor(a.user_id());
        double factor_b = fair_share_->get_priority_factor(b.user_id());
        
        double prio_a = a.spec().priority() * factor_a;
        double prio_b = b.spec().priority() * factor_b;
        
        if (std::abs(prio_a - prio_b) > 0.1) {
            return prio_a > prio_b; // Higher priority first
        }
        return a.submitted_at() < b.submitted_at(); // Older first (FIFO)
    });

    auto nodes = node_registry_->get_all_nodes();
    std::vector<NodeInfo> online_nodes;
    for (const auto& node : nodes) {
        if (node.status() == NODE_STATUS_ONLINE) {
            online_nodes.push_back(node);
        }
    }

    if (online_nodes.empty()) {
        // No nodes available
        return;
    }

    for (const auto& job : pending_jobs) {
        bool scheduled = false;
        const auto& req = job.spec().resources();

        for (auto& node : online_nodes) {
            // Check if node has enough available resources (using its reported available resources)
            // In a full implementation, the ResourceManager would handle this reliably.
            // For Phase 1, we just do a simplistic check.
            const auto& avail = node.available_resources();
            
            if (req.cpu_cores() <= avail.cpu_cores() && 
                req.ram_mb() <= avail.ram_mb() && 
                req.gpu_count() <= avail.gpu_count()) {
                
                // Assign job to this node
                job_mgr_->update_job_status(job.job_id(), JOB_STATUS_ASSIGNED, node.node_id());
                LOG_INFO("Scheduler assigned job {} to node {}", job.job_id(), node.node_id());
                
                // Deduct resources locally so we can schedule multiple jobs in this pass
                // We're modifying our local copy of online_nodes. The real NodeRegistry will
                // be updated either by node heartbeats or by our explicit allocation (Phase 3).
                node.mutable_available_resources()->set_cpu_cores(avail.cpu_cores() - req.cpu_cores());
                node.mutable_available_resources()->set_ram_mb(avail.ram_mb() - req.ram_mb());
                node.mutable_available_resources()->set_gpu_count(avail.gpu_count() - req.gpu_count());
                
                scheduled = true;
                break;
            }
        }

        if (!scheduled) {
            // Could not schedule this job. Wait for next pass.
            // In a real backfill scheduler, we would compute a reservation here and look
            // at subsequent smaller jobs.
        }
    }
}

} // namespace certosc
