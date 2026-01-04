#pragma once

#include <vector>
#include <string>
#include <fstream>

class StatisticsTracker {
public:
    StatisticsTracker();
    ~StatisticsTracker();

    // Controls the overall logging session
    void startNewLog(const std::string& filename);
    void stopLogging();
    bool isLogging() const;

    // Data handling methods (used by the simulation during a run)
    void record(int step, long magnetization, long energy);
    void reset(); // Clears in-memory data

    // Data access methods - used by ImPlot for real-time plotting
    const std::vector<double>& getSteps() const;
    const std::vector<double>& getMagnetizations() const;
    const std::vector<double>& getEnergies() const;
    const std::vector<double>& getEnergyVariances() const;

    // Methods to get final statistics from the buffered data
    double getAverageMagnetisation() const;
    double getAverageEnergy() const;
    double getEnergyVariance() const;
    bool hasData() const;

private:
    void flushToFile();
    void clearBuffers();

    bool m_is_logging;

    // Configuration
    size_t m_batch_size;

    // File Handling
    std::ofstream m_output_file;

    // In-memory data buffers (for plotting)
    std::vector<double> m_steps;
    std::vector<double> m_magnetizations;
    std::vector<double> m_energies;
    std::vector<double> m_energy_variances;

    // Running Statistics (persistent across flushes)
    double m_sum_magnetization;
    double m_sum_energy;
    double m_sum_energy_sq; // Sum of energies squared
    unsigned long m_count;
};
