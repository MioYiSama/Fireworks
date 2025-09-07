#include "util.h"

float random() {
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_real_distribution dist(0.0f, 1.0f);

    return dist(mt);
}
