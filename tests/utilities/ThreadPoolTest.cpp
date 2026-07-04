#include <gtest/gtest.h>
#include <utilities/ThreadPool.h>
#include <atomic>

using namespace rav;

TEST(ThreadPoolTest, EnqueueAndWait) {
    ThreadPool pool(2);
    auto future = pool.enqueue([] { return 42; });
    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPoolTest, MultipleTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    std::vector<std::future<void>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.enqueue([&] { ++counter; }));
    }
    for (auto& f : futures) f.get();

    EXPECT_EQ(counter, 100);
}

TEST(ThreadPoolTest, WorkerCount) {
    ThreadPool pool(4);
    EXPECT_EQ(pool.worker_count(), 4);
}
