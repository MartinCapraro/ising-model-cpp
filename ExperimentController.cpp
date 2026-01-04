#include "ExperimentController.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

// Experiment config
constexpr int EXPERIMENT_L = 40;
constexpr int EQUILIBRATION_STEPS = 1000;
constexpr int MEASUREMENT_STEPS = 4000;


ExperimentController::ExperimentController(const std::vector<double>& temperatures)
    : m_temperatures(temperatures)
{
}

void ExperimentController::run()
{
    std::cout << "\nStarting experiment..." << std::endl;
    m_results.clear();

    for (const double temp : m_temperatures) {
        runSimulationAt(temp);
    }

    std::cout << "\nExperiment complete!" << std::endl;
    saveSummaryToFile("experiment_summary.csv");
}

void ExperimentController::runSimulationAt(double temperature)
{
    std::cout << "Running simulation for T = " << temperature << "." << std::endl;

    auto run_start_time = std::chrono::high_resolution_clock::now();

    m_simulation.reconfigure(EXPERIMENT_L, temperature);

    //  Equilibrate: Run the simulation for a while to let it settle
    std::cout << "Equilibration phase started, running for " << EQUILIBRATION_STEPS << " steps." << std::endl;
    for (int i = 0; i < EQUILIBRATION_STEPS; ++i) {
        m_simulation.advanceEquilibrationStep();
    }

    auto equilibration_end_time = std::chrono::high_resolution_clock::now();

    // Measure: Reset the stats, then run while recording data
    m_simulation.resetStatistics();
    std::cout << "Measurement step start, running for " << MEASUREMENT_STEPS << " steps." << std::endl;
    for (int i = 0; i < MEASUREMENT_STEPS; ++i) {
        m_simulation.performMeasurementStep();
    }
    auto measurement_end_time = std::chrono::high_resolution_clock::now();
    std::cout << "Measurement step completed." << std::endl;

    // Collect the final averaged results for this temperature
    m_results.push_back(m_simulation.getFinalStatistics());

    auto equilibration_duration = std::chrono::duration_cast<std::chrono::milliseconds>(equilibration_end_time - run_start_time);
    auto measurement_duration = std::chrono::duration_cast<std::chrono::milliseconds>(measurement_end_time - equilibration_end_time);

    std::cout << "Equilibration took: " << equilibration_duration.count()/1000 << " s." << std::endl;
    std::cout << "Measurement took: " << measurement_duration.count()/1000 << " s.\n" << std::endl;
}

void ExperimentController::saveSummaryToFile(const std::string& filename) const
{
    std::cout << "Saving summary to " << filename << "..." << std::endl;
    std::ofstream summary_file(filename);

    if (!summary_file.is_open()) {
        std::cerr << "Error: Could not open summary file " << filename << std::endl;
        return;
    }

    // Write header
    summary_file << "Temperature,MagnetisationPerSpin,EnergyPerSpin,HeatCapacityPerSpin\n";

    // Write data
    for (const auto& result : m_results) {
        summary_file << result.temperature << ","
                     << result.avg_magnetisation << ","
                     << result.avg_energy << ","
                     << result.heat_capacity << "\n";
    }

    std::cout << "Save complete." << std::endl;
}
