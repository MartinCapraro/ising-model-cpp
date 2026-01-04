#include "StatisticsTracker.h"
#include <iostream>


StatisticsTracker::StatisticsTracker()
    : m_is_logging(false), // Collect 10,000 records in memory before flushing to file.
      m_batch_size(10000),
      m_sum_magnetization(0.0),
      m_sum_energy(0.0),
      m_sum_energy_sq(0.0),
      m_count(0)
{
    // Pre-allocate memory in the vectors to avoid frequent reallocations.
    m_steps.reserve(m_batch_size);
    m_magnetizations.reserve(m_batch_size);
    m_energies.reserve(m_batch_size);
}

// Ensures any remaining data is saved when the tracker is destroyed.
StatisticsTracker::~StatisticsTracker()
{
    stopLogging();
}

// Opens a new file for logging and writes the header.
void StatisticsTracker::startNewLog(const std::string& filename)
{
    // If a file is already open from a previous run, close it.
    if (m_output_file.is_open()) {
        m_output_file.close();
    }

    m_output_file.open(filename);
    if (!m_output_file.is_open()) {
        std::cerr << "Error: Failed to open statistics file: " << filename << std::endl;
        return;
    }

    // Write the header row for the CSV file.
    m_output_file << "Step,Magnetization,Energy\n";
    m_is_logging = true;
}

// Flushes any remaining data and closes the log file.
void StatisticsTracker::stopLogging()
{
    flushToFile();
    if (m_output_file.is_open()) {
        m_output_file.close();
    }
    m_is_logging = false;
}

// To allow other parts of the application to read the logging status
bool StatisticsTracker::isLogging() const {
    return m_is_logging;
}

// Main method for adding a new data point.
void StatisticsTracker::record(const int step, const long magnetization, const long energy)
{
    double mag_d = static_cast<double>(magnetization);
    double eng_d = static_cast<double>(energy);


    // Accumulate persistent statistics
    m_sum_magnetization += mag_d;
    m_sum_energy += eng_d;
    m_sum_energy_sq += (eng_d * eng_d);
    m_count++;

    // Calculate the current variance (running), Var(E) = <E^2> - <E>^2
    double mean = m_sum_energy / static_cast<double>(m_count);
    double mean_sq = m_sum_energy_sq / static_cast<double>(m_count);
    double current_variance = mean_sq - (mean * mean);

    // Add the new data to our in-memory buffers (for plotting)
    m_steps.push_back(static_cast<double>(step));
    m_magnetizations.push_back(mag_d);
    m_energies.push_back(eng_d);
    m_energy_variances.push_back(current_variance);

    // If the buffer has reached the batch size, flush it to the file.
    if (m_steps.size() >= m_batch_size) {
        flushToFile();
    }
}


// Clears the in-memory buffers. Called when starting a new simulation run.
void StatisticsTracker::reset()
{
    clearBuffers();

    // Reset accumulators
    m_sum_magnetization = 0.0;
    m_sum_energy = 0.0;
    m_sum_energy_sq = 0.0;
    m_count = 0;
}

// Clears the temporary vectors used for file IO and plotting
void StatisticsTracker::clearBuffers()
{
    m_steps.clear();
    m_magnetizations.clear();
    m_energies.clear();
    m_energy_variances.clear();
}

// Data access methods for the GUI (used by ImPlot)
const std::vector<double>& StatisticsTracker::getSteps() const {
    return m_steps;
}
const std::vector<double>& StatisticsTracker::getMagnetizations() const {
    return m_magnetizations;
}

const std::vector<double>& StatisticsTracker::getEnergies() const {
    return m_energies;
}

const std::vector<double>& StatisticsTracker::getEnergyVariances() const {
    return m_energy_variances;
}

// Writes the contents of the in-memory buffers to the file and clears them.
void StatisticsTracker::flushToFile()
{
    if (!m_output_file.is_open() || !m_output_file.good()) {
        return;
    }

    for (size_t i = 0; i < m_steps.size(); ++i) {
        m_output_file << m_steps[i] << "," << m_magnetizations[i] << "," << m_energies[i] << "\n";
    }

    // Only clear the buffers, do NOT reset the running statistics
    clearBuffers();
}

bool StatisticsTracker::hasData() const {
    return !m_steps.empty();
}

double StatisticsTracker::getAverageMagnetisation() const {
    if (m_magnetizations.empty()) return 0.0;
    return m_sum_magnetization / static_cast<double>(m_count);
}

double StatisticsTracker::getAverageEnergy() const {
    if (m_energies.empty()) return 0.0;
    return m_sum_energy / static_cast<double>(m_count);
}

double StatisticsTracker::getEnergyVariance() const {
    if (m_count == 0) return 0.0;

    double mean = m_sum_energy / static_cast<double>(m_count);
    double mean_sq = m_sum_energy_sq / static_cast<double>(m_count);

    return mean_sq - (mean * mean);
}

