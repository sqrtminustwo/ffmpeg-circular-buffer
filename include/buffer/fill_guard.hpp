#ifndef FILL_GUARD_H
#define FILL_GUARD_H

#include <atomic>
#include <condition_variable>

enum RefillType { PARTIAL, FULL, HALF, ARG };

class FillGuard {
    std::atomic_bool &filling;
    std::condition_variable &cv;

  public:
    FillGuard(std::atomic_bool &filling, std::condition_variable &cv);
    ~FillGuard();

    FillGuard(const FillGuard &) = delete;
    FillGuard &operator=(const FillGuard &) = delete;
    FillGuard(FillGuard &&) = delete;
    FillGuard &operator=(FillGuard &&) = delete;
};

#endif
