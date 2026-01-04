#pragma once

#include <SDL2/SDL.h>
#include <string>
#include "StatisticsTracker.h"

// Used in the headless mode when the ising program is called with the --experiment flag
struct SimulationResult {
    double temperature;
    double avg_magnetisation;
    double avg_energy;
    double heat_capacity;
    // TODO: add heat capacity and whatever else... correlation length?
};

class IsingSimulation {
public:
    enum class State {
        Paused,
        Running,
        Stopped
    };

    IsingSimulation();
    ~IsingSimulation();

    // Interface for the application to control the simulation
    void start();
    void stop();
    void pause();   // New
    void resume();
    void reconfigure(int L, double T);
    void setTemperature(double T);
    void startLogging(const std::string& filename);
    void stopLogging();


    // Interface for synchronization with the render thread
    void waitForCompute();
    void signalRendererDone();

    bool isThreadActive() const;

    // Getter interfaces
    const int* getLatticeState() const;
    int getStep() const;
    std::string getTemperatureString() const;
    int getLatticeDimension() const;
    State getState() const;
    bool isLogging() const;

    const std::vector<double>& getSteps() const;
    const std::vector<double>& getMagnetizations() const;
    const std::vector<double>& getEnergies() const;
    const std::vector<double>& getEnergyVariances() const;
    double getTemperature() const { return m_T; }

    // Used in the headless mode
    void advanceEquilibrationStep();
    void performMeasurementStep();
    void resetStatistics();
    SimulationResult getFinalStatistics() const;

private:
    void initialize();
    void sweep();
    int sum_of_neighbouring_spins(int i);
    bool flip_or_not(int i, int delta_e);
    int compute_loop_impl();

    // Simulation state
    int* m_lattice;     // lattice
    int m_L;      // lattice length
    double m_prob[5];   // for storing probabilities
    int m_N;
    double m_T;         // temperature
    int m_step;
    // Computed state
    int* m_E;           // energy
    long m_E_total;     // total energy
    long m_M_total;     // total magnetisation

    static constexpr double k = 1.0;
    static constexpr int J = 1;                     // Interaction energy (positive for ferromagnetism, negative for paramagnetism)
    static constexpr int INITIALIZE_UP = 0;
    static constexpr double RUNS_PER_SWEEP = 1e6;   // Number of runs (attempted spin flips)

    // Threading primitives
    SDL_mutex* m_mutex;
    SDL_cond* m_cond_compute_done;
    SDL_cond* m_cond_render_done;
    SDL_Thread* m_compute_thread;

    // Threading flags
    bool m_compute_done;
    bool m_render_done;

    // State enum
    State m_state;
    bool m_is_thread_active;

    // The member object responsible for tracking statistics;
    StatisticsTracker m_stats_tracker;

public:
    static int thread_entry(void* p);
};