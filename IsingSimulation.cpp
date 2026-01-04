#include "IsingSimulation.h"
#include "random.h"    // For the mt_rand_gen used in initialize()

#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>


IsingSimulation::IsingSimulation()
    : m_lattice(nullptr),
      m_L(500),
      m_prob{},         // Use {} to default-initialize the array
      m_N(m_L * m_L),
      m_T(2.0),
      m_step(0),
      m_E(nullptr),
      m_E_total(0),
      m_M_total(0),
      m_mutex(nullptr),
      m_cond_compute_done(nullptr),
      m_cond_render_done(nullptr),
      m_compute_thread(nullptr),
      m_compute_done(false),
      m_render_done(true),
      m_state(State::Paused),
      m_is_thread_active(false)
{
    initialize();

    // Create the synchronization primitives that belong to the simulation
    m_mutex = SDL_CreateMutex();
    m_cond_compute_done = SDL_CreateCond();
    m_cond_render_done = SDL_CreateCond();

    if (!m_mutex || !m_cond_compute_done || !m_cond_render_done) {
        std::cerr << "Failed to create simulation synchronization primitives: " << SDL_GetError() << std::endl;
        // In a real app, you'd want to handle this failure, perhaps with an exception
    }

    // Set file to log statistics to
    m_stats_tracker.startNewLog("ising_stats.csv");
}


IsingSimulation::~IsingSimulation()
{
    if (m_compute_thread) {
        SDL_WaitThread(m_compute_thread, nullptr);
    }
    // Free the memory allocated for the lattice
    delete[] m_lattice;
    delete[] m_E;

    // Destroy the synchronization primitives
    if (m_cond_render_done) {
        SDL_DestroyCond(m_cond_render_done);
    }
    if (m_cond_compute_done) {
        SDL_DestroyCond(m_cond_compute_done);
    }
    if (m_mutex) {
        SDL_DestroyMutex(m_mutex);
    }
}


void IsingSimulation::initialize()
{
    std::cout << "Initialising simulation...";
    int i;
    int sum;
    double rand_num;


    m_lattice = new int[static_cast<size_t>(m_N)];
    m_E = new int[static_cast<size_t>(m_N)];

    m_E_total = 0;
    m_M_total = 0;
    
    // NB: we only use two of these elements as a hack
    // to make the logic a bit simpler when deciding whether
    // a site needs to be flipped
    for (i = 2; i < 5; i += 2) {
        double beta = 1.0/(k * m_T);
        m_prob[i] = exp(-2.0 * beta * i);
    }
    
    /* Set initial lattice state */
    if(INITIALIZE_UP == 1) {
        for (i = 0; i < m_N; i++) {
            m_lattice[i] = 1;
        }
    }
    else {
    // Random initialisation
        for (i = 0; i < m_N; i++) {
            rand_num = drand();
            m_lattice[i] = (rand_num < 0.5) ? -1 : 1;
        }
    }
    
    // Calculate energy of initial lattice (with helical boundary conditions)
    for(i = 0; i < m_N - 1; i++) {
        if( (i - 1*(i % m_L))/m_L == (m_L - 1) ) {
          sum = m_lattice[i+1] + m_lattice[i % m_L];
        }
        else {
          sum = m_lattice[i+1] + m_lattice[i + m_L];
        }
        m_E[i] = -J * m_lattice[i] * sum;
        m_E_total += m_E[i];
        m_M_total += m_lattice[i];
    }
    
    sum = m_lattice[0] + m_lattice[m_L - 1];
    m_E[m_N - 1] = -J * m_lattice[m_N - 1] * sum;
    m_E_total += m_E[m_N-1];
    m_M_total += m_lattice[m_N-1];

    m_step = 0;
}

void IsingSimulation::sweep()
{
    //int i;
    bool update;
    int delta_e;
    int sum=0;

    for (int j = 0; j < RUNS_PER_SWEEP; j++) {
        // Choose a random site
        auto i = static_cast<int>(m_N*drand());

        // Calculate the sum of the neighbouring spins, taking helical boundary conditions into account
        sum = sum_of_neighbouring_spins(i);

        // The change in energy if the spin is flipped
        delta_e = sum*m_lattice[i];

        // Decide whether to flip the spin, and update energy and magnetization accordingly
        update = flip_or_not(i, delta_e);

        if(update) {
            m_E_total += 2*delta_e;
        }
  }
}

bool IsingSimulation::flip_or_not(int const i, int const delta_e) {
  bool update = false;

  if (delta_e <= 0) {
    if(m_lattice[i] > 0) {
      m_M_total = m_M_total - 2;
    }
    else {
      m_M_total = m_M_total + 2;
    }
    m_lattice[i] = -m_lattice[i];
    update = true;
  }
  else if (drand() < m_prob[delta_e]) {
    if(m_lattice[i] > 0) {
      m_M_total = m_M_total - 2;
    }
    else {
      m_M_total = m_M_total + 2;
    }
    m_lattice[i] = -m_lattice[i];
    update = true;
  }
  return update;
}

int IsingSimulation::sum_of_neighbouring_spins(int i)
{
    int nn;
    int sum = 0;

    // Right neighbour
    if ((nn = i + 1) >= m_N) {
      nn -= m_N;
    }
    sum = m_lattice[nn];

    // Left neighbour
    if ((nn = i - 1) <  0) {
      nn += m_N;
    }
    sum += m_lattice[nn];

    // Bottom neighbour
    if ((nn = i + m_L) >= m_N) {
      nn -= m_N;
    }
    sum += m_lattice[nn];

    // Top neighbour
    if ((nn = i - m_L) <  0) {
      nn += m_N;
    }
    sum += m_lattice[nn];

    return sum;
}


void IsingSimulation::start()
{
    m_is_thread_active = true;
    m_state = State::Running;
    m_compute_thread = SDL_CreateThread(IsingSimulation::thread_entry, "computeThread", this);
}

void IsingSimulation::stop()
{
    SDL_LockMutex(m_mutex);
    m_state = State::Stopped;
    m_is_thread_active = false;
    SDL_CondSignal(m_cond_render_done);
    // Set all the spins to down
    for (int i = 0; i < m_N; ++i) {
        m_lattice[i] = -1;
    }
    m_step = 0;
    SDL_UnlockMutex(m_mutex);
}

void IsingSimulation::pause()
{
    SDL_LockMutex(m_mutex);
    m_state = State::Paused;
    SDL_UnlockMutex(m_mutex);
}

void IsingSimulation::resume()
{
    SDL_LockMutex(m_mutex);
    m_state = State::Running;
    // Signal the condition variable to wake up the thread if it's paused
    SDL_CondSignal(m_cond_render_done);
    SDL_UnlockMutex(m_mutex);
}

void IsingSimulation::reconfigure(int L, double T)
{
    // Stop the current thread
    if (m_compute_thread) {
        stop();
        SDL_WaitThread(m_compute_thread, nullptr);
        m_compute_thread = nullptr;
    }

    // Deallocate old memory
    delete[] m_lattice;
    delete[] m_E;

    // Update parameters
    m_L = L;
    m_N = L * L;
    m_T = T;

    // Re-initialize with the new parameters and set the state to paused
    initialize();
    m_state = State::Paused;

    // Clear the data from the tracker's in-memory buffers.
    m_stats_tracker.reset();
}

// A thread-safe method to change the temperature
void IsingSimulation::setTemperature(double T)
{
    SDL_LockMutex(m_mutex);
    m_T = T;
    // Recalculate probabilities based on the new temperature
    for (int i = 2; i < 5; i += 2) {
        double beta = 1.0 / (k * m_T);
        m_prob[i] = exp(-2.0 * beta * i);
    }
    SDL_UnlockMutex(m_mutex);
}

IsingSimulation::State IsingSimulation::getState() const
{
    return m_state;
}

// Synchronization methods
void IsingSimulation::waitForCompute()
{
    // Lock the mutex and wait for the compute thread to finish its step
    SDL_LockMutex(m_mutex);
    while (!m_compute_done) {
        SDL_CondWait(m_cond_compute_done, m_mutex);
    }
}

void IsingSimulation::signalRendererDone()
{
    // Set the flags, signal the compute thread to wake up, and unlock the mutex.
    m_compute_done = false;
    m_render_done = true;
    SDL_CondSignal(m_cond_render_done);
    SDL_UnlockMutex(m_mutex);
}


const int* IsingSimulation::getLatticeState() const
{
    return m_lattice;
}

int IsingSimulation::getStep() const
{
    return m_step;
}

std::string IsingSimulation::getTemperatureString() const
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << m_T;
    return stream.str();
}


// Threading things

int IsingSimulation::thread_entry(void* p)
{
    return static_cast<IsingSimulation*>(p)->compute_loop_impl();
}

// The real thread loop
int IsingSimulation::compute_loop_impl()
{
    while(m_is_thread_active)
    {
        SDL_LockMutex(m_mutex);

        // The thread sleeps here if it's waiting for the renderer, or if it's paused.
        while ((!m_render_done || m_state == State::Paused) && m_is_thread_active) {
            SDL_CondWait(m_cond_render_done, m_mutex);
        }

        // If stop() was called while we were waiting, exit
        // TODO: can this ever be hit??
        if (!m_is_thread_active) {
            SDL_UnlockMutex(m_mutex);
            break;
        }
      
      sweep();
      m_step++;

      // Record the data for the completed sweep step
      m_stats_tracker.record(m_step, m_M_total, m_E_total);

      m_compute_done = true;
      m_render_done = false;

      SDL_CondSignal(m_cond_compute_done);
      SDL_UnlockMutex(m_mutex);
    }

    return 0;
}

int IsingSimulation::getLatticeDimension() const {
    return m_L;
}

bool IsingSimulation::isThreadActive() const
{
    return m_is_thread_active;
}

void IsingSimulation::startLogging(const std::string& filename)
{
    m_stats_tracker.startNewLog(filename);
}

void IsingSimulation::stopLogging()
{
    m_stats_tracker.stopLogging();
}

bool IsingSimulation::isLogging() const
{
    return m_stats_tracker.isLogging();
}

const std::vector<double>& IsingSimulation::getSteps() const
{
    return m_stats_tracker.getSteps();
}

const std::vector<double>& IsingSimulation::getMagnetizations() const
{
    return m_stats_tracker.getMagnetizations();
}

const std::vector<double>& IsingSimulation::getEnergies() const
{
    return m_stats_tracker.getEnergies();
}

const std::vector<double>& IsingSimulation::getEnergyVariances() const
{
    return m_stats_tracker.getEnergyVariances();
}

// Used in the headless mode: advance the simulation without recording data
void IsingSimulation::advanceEquilibrationStep() {
    sweep();
    m_step++;
}

// Used in the headless mode::  advance the simulation AND record the statistics
void IsingSimulation::performMeasurementStep() {
    sweep();
    m_step++;
    m_stats_tracker.record(m_step, m_M_total, m_E_total);
}

// Used in the headless mode
void IsingSimulation::resetStatistics() {
    m_stats_tracker.reset();
}

// Used in the headless mode
SimulationResult IsingSimulation::getFinalStatistics() const {
    SimulationResult result;
    result.temperature = m_T;

    // Normalise per spin
    result.avg_magnetisation = m_stats_tracker.getAverageMagnetisation() / m_N;
    result.avg_energy = m_stats_tracker.getAverageEnergy() / m_N;


    // Calculate heat apacity: Cv = Variance(E) / (T^2), and normalise per spin
    double total_variance = m_stats_tracker.getEnergyVariance();
    result.heat_capacity = total_variance / (m_T * m_T * m_N);

    return result;
}