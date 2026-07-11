#include "buffer/fill_guard.hpp"

FillGuard::FillGuard(std::atomic_bool &filling, std::condition_variable &cv)
    : filling{filling}, cv{cv} {
    this->filling = true;
}

FillGuard::~FillGuard() {
    if (filling) {
        filling = false;
        cv.notify_all();
    }
}
