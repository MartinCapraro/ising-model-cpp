#pragma once

#include "IsingSimulation.h"
#include <vector>
#include <string>

// For running the Ising simulation in a headless mode, for a bunch of different temperatures
class ExperimentController {
public:
    explicit ExperimentController(const std::vector<double>& temperatures);

    // Main entry point
    void run();

private:
    void runSimulationAt(double temperature);
    void saveSummaryToFile(const std::string& filename) const;

    IsingSimulation m_simulation;
    std::vector<double> m_temperatures;
    std::vector<SimulationResult> m_results;
};