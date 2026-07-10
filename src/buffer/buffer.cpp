#include "buffer/buffer.hpp"
#include <algorithm>

int Buffer::normalize_size(int size) { return std::min(size, (int)total_size); }
int Buffer::normalize_size_default() { return normalize_size(total_size - offset); }
