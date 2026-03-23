#include "random.h"

uint64_t RNGU64::next() {
    return engine();
}