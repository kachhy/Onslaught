#include "movelist.h"

void MoveList::sort_item(const uint8_t index) {
    std::swap(list[index], list[sel_sort_index]);
    sel_sort_index++;
}