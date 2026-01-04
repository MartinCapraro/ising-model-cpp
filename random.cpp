#include "random.h"
#include <random>


double drand() {
    static std::random_device rd;          // Non-deterministic random device
    static std::mt19937 engine(rd());      // Mersenne Twister engine
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);  // Uniform distribution between 0 and 1

    // Generate and return a random number in [0, 1)
    return distribution(engine);
}
