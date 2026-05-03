#include <gtest/gtest.h>
#include "certosc/master/scheduler/fair_share.h"
#include <thread>
#include <chrono>

using namespace certosc;

TEST(FairShareTest, InitialPriorityFactorIsOne) {
    FairShareManager mgr(24.0); // 24 hour half-life
    EXPECT_DOUBLE_EQ(mgr.get_priority_factor("user1"), 1.0);
}

TEST(FairShareTest, UsageReducesPriority) {
    FairShareManager mgr(24.0);
    
    // Add 10 hours of CPU usage
    mgr.add_usage("user1", 36000.0);
    
    double factor = mgr.get_priority_factor("user1");
    EXPECT_LT(factor, 1.0);
    EXPECT_GT(factor, 0.0);
}

TEST(FairShareTest, DecayRestoresPriority) {
    // Very short half-life to test decay logic quickly
    // Note: Our implementation decays based on real time `now_unix()`.
    // In a pure unit test, we should abstract time, but for Phase 4 MVP we'll just test logic loosely.
    FairShareManager mgr(0.0001); // highly accelerated half-life
    
    mgr.add_usage("user1", 36000.0);
    double initial_factor = mgr.get_priority_factor("user1");
    
    // Sleep to allow time to pass
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    mgr.apply_decay();
    
    double new_factor = mgr.get_priority_factor("user1");
    EXPECT_GT(new_factor, initial_factor); // Factor should increase (closer to 1.0) as usage decays
}
