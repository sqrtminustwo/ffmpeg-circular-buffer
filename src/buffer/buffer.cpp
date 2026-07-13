#include "buffer/buffer.hpp"
#include <algorithm>

void Buffer::set_base(uint8_t *base) { this->base = base; }
uint8_t *Buffer::get_base() { return base; }
int Buffer::get_offset() { return offset.load(); }
void Buffer::set_offset(int offset) { this->offset.exchange(offset); }
size_t Buffer::get_total_size() { return total_size.load(); }
void Buffer::set_total_size(size_t total_size) { this->total_size.exchange(total_size); }

int Buffer::normalize_size(int size) { return std::min(size, (int)total_size); }
int Buffer::normalize_size_default() { return normalize_size(total_size - offset); }
