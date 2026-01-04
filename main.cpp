/*********************************************************************/
/*

This is the main entry point for a simulation of the Ising model. It
uses the Metropolis algorithm, with helical boundary conditions, as 
published in Newman and Barkema's "Monte Carlo Methods in Statistical
Physics" on page 434.


COMPILATION:

If you are on a Unix-like system (e.g. Linux, and maybe MacOS) you should
simply be able to run 'make' to get the program to compile, and run
the resulting executable.

You will require the build-essential, libglew-dev, libsdl2-dev, and libglm-dev
libraries on linux, or the sdl2, glfw, glew, and glm libraries on macos.

By default the makefile will try to compile modern OpenGL. If this is
not available to you, or if for whatever reason you would prefer
legacy OpenGL, set MODERN_GL_ENABLED = 0 in the makefile.


*/
/*********************************************************************/

/* STANDARD INCLUDES */
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>

#include <SDL2/SDL.h>
#include <GL/glew.h>

#include "IsingSimulation.h"
#include "ExperimentController.h"
#include "rendering/Renderer.h"
#include "rendering/LegacyGLRenderer.h"
#include "rendering/ModernGLRenderer.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "implot.h"

// Conditionally include the correct OpenGL backend header
#ifdef MODERN_GL
    #include "imgui_impl_opengl3.h"
#else
    #include "imgui_impl_opengl2.h"
#endif



class Application {
public:
    Application();
    ~Application();
    void run();
    bool isInitialized() const { return m_is_initialized; }

private:
    int m_width;
    int m_height;
    static bool init() ;
    bool createWindow();
    void initGui();
    void drawGui();
    void drawControlsWindow();
    void drawSimulationParametersControls();
    void drawLoggingControls();
    void drawPlottingWindow();

    SDL_Window* m_window;
    SDL_GLContext m_gl_context;
    std::unique_ptr<Renderer> m_renderer;
   
    // NB: the application owns the simulation as a single object 
    IsingSimulation m_simulation;

    bool m_running;
    bool m_is_initialized;

    // To hold the state of the GUI controls
    int m_gui_L;
    float m_gui_T;
    char m_gui_log_filename[128];
    bool m_should_fit_plots;
    
};


// Constructor
Application::Application()
    : m_width(1000), // Initial width
      m_height(1000), // Initial height
      m_window(nullptr),
      m_gl_context(nullptr),
      m_running(false),
      m_is_initialized(false),
      m_gui_L(500),
      m_gui_T(2.0f),
      m_should_fit_plots(true)
{
    strncpy(m_gui_log_filename, "ising_stats.csv", 128);
    // Explicitly specify a default null terminated string for safety
    m_gui_log_filename[127] = '\0';

    if (init() && createWindow()) {
        m_is_initialized = true;
    }
}


// Destructor
Application::~Application()
{
    std::cout << "Application shutting down..." << std::endl;

    // Shutdown ImGui
    #ifdef MODERN_GL
        ImGui_ImplOpenGL3_Shutdown();
    #else
        ImGui_ImplOpenGL2_Shutdown();
    #endif
    ImGui_ImplSDL2_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    // Destroy the OpenGL context
    if (m_gl_context) {
        SDL_GL_DeleteContext(m_gl_context);
    }

    // Destroy the window
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }

    // Quit all SDL subsystems
    SDL_Quit();
}


void Application::run()
{
    m_running = true;

    while(m_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                m_running = false;
            }
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(m_window)) {
                m_running = false;
            }
            // listen for window resizing events
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                m_width = event.window.data1;
                m_height = event.window.data2;
                glViewport(0, 0, m_width, m_height);
            }
        }

        // Start the Dear ImGui frame
        #ifdef MODERN_GL
                ImGui_ImplOpenGL3_NewFrame();
        #else
                ImGui_ImplOpenGL2_NewFrame();
        #endif
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        drawGui();

        SDL_GL_MakeCurrent(m_window, m_gl_context);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::string baseTitle = "Ising model (T = " + m_simulation.getTemperatureString() + ") step ";
        std::stringstream titleStream;
        titleStream << baseTitle << m_simulation.getStep();
        SDL_SetWindowTitle(m_window, titleStream.str().c_str());

        // Check the simulation state to decide whether to advance the frame.
        if (m_simulation.getState() == IsingSimulation::State::Running) {
            // Wait for the compute thread to finish, then signal when we are done drawing.
            m_simulation.waitForCompute();

            if (m_renderer) {
                m_renderer->draw(m_simulation.getLatticeState(), m_simulation.getLatticeDimension());
            }
            m_simulation.signalRendererDone();
        } else {
            // If paused or stopped, just draw the last known state of the lattice without synchronizing.
            if (m_renderer) {
                m_renderer->draw(m_simulation.getLatticeState(), m_simulation.getLatticeDimension());
            }
        }

        ImGui::Render();
        #ifdef MODERN_GL
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #else
                ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        #endif

        // Handle Viewports
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }

        SDL_GL_SwapWindow(m_window);
        SDL_Delay(8);

    }
    m_simulation.stop();
}


bool Application::init() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    #ifdef MODERN_GL
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    #endif

    return true;
}

bool Application::createWindow() {
    std::string title = "Ising model (T = " + m_simulation.getTemperatureString() + ") step " + std::to_string(m_simulation.getStep());

    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_width,
        m_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!m_window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    m_gl_context = SDL_GL_CreateContext(m_window);
    if (!m_gl_context) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL version: " << version << std::endl;

    initGui();

    #ifdef MODERN_GL
        m_renderer = std::make_unique<ModernGLRenderer>();
    #else
        // Set up a coordinate system from (0,0) to (L,L)
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity(); // Reset the projection matrix
        glOrtho(0, m_simulation.getLatticeDimension(), m_simulation.getLatticeDimension(), 0, -1, 1);
        m_renderer = std::make_unique<LegacyGLRenderer>();
    #endif

    return true;
}

void Application::initGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // Set font size
    io.Fonts->AddFontFromFileTTF("assets/Roboto-Medium.ttf", 20.0f);
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(m_window, m_gl_context);

    // Conditionally initialize the correct renderer backend
    #ifdef MODERN_GL
        // The GLSL version string should match the OpenGL version.
        // For GL 3.3 it's "#version 330"
        const char* glsl_version = "#version 330";
        ImGui_ImplOpenGL3_Init(glsl_version);
    #else
        ImGui_ImplOpenGL2_Init();
    #endif
}

void Application::drawGui()
{
    drawControlsWindow();
    drawPlottingWindow();
}

void Application::drawControlsWindow()
{
    ImGui::Begin("Controls");
    // Get the simulation's status once per frame
    const bool is_thread_active = m_simulation.isThreadActive();

    // Display info
    ImGui::Text("Step: %d", m_simulation.getStep());
    if (!is_thread_active) {
        ImGui::Text("State: Stopped");
    } else {
        ImGui::Text("State: %s", (m_simulation.getState() == IsingSimulation::State::Running) ? "Running" : "Paused");
    }
    ImGui::Separator();

    drawSimulationParametersControls();
    drawLoggingControls();

    // Context-aware GUI control buttons
    if (m_simulation.isThreadActive()) {
        // If the thread is active, we can pause or resume it.
        if (m_simulation.getState() == IsingSimulation::State::Running) {
            if (ImGui::Button("Pause")) {
                m_simulation.pause();
            }
        } else {
            if (ImGui::Button("Resume")) {
                m_simulation.resume();
            }
        }
        ImGui::SameLine();
        // The Stop button will terminate the thread completely.
        if (ImGui::Button("Stop")) {
            m_simulation.stop();
        }

    } else {
        // If there is no thread, the only option is to start a new simulation.
        if (ImGui::Button("Start Simulation")) {
            m_simulation.reconfigure(m_gui_L, static_cast<double>(m_gui_T));
            m_simulation.start();
            // Reset this every time we start a new simulation
            m_should_fit_plots = true;
        }
    }

    ImGui::End();
}

void Application::drawSimulationParametersControls()
{
    const bool is_thread_active = m_simulation.isThreadActive();
    // Lattice size controls
    ImGui::Text("Simulation Parameters");
    ImGui::BeginDisabled(is_thread_active);
    ImGui::SetNextItemWidth(150.0f); // Make the slider a bit smaller
    ImGui::SliderInt("Lattice size", &m_gui_L, 32, 1024);
    ImGui::SameLine(); // Place the next widget on the same line
    ImGui::SetNextItemWidth(100.0f); // Make the input box a bit smaller
    ImGui::InputInt("L", &m_gui_L);
    ImGui::EndDisabled();

    // Temperature controls
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::SliderFloat("Temperature", &m_gui_T, 0.1f, 5.0f)) {
        m_simulation.setTemperature(static_cast<double>(m_gui_T));
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputFloat("T", &m_gui_T)) {
        m_simulation.setTemperature(static_cast<double>(m_gui_T));
    }

    ImGui::Separator();

}

void Application::drawLoggingControls()
{
    ImGui::Separator();
    ImGui::Text("Data Logging");
    const bool is_logging = m_simulation.isLogging();
    ImGui::BeginDisabled(is_logging); // Disable filename input while logging
    ImGui::InputText("Filename", m_gui_log_filename, 128);
    ImGui::EndDisabled();
    ImGui::SameLine();

    if (is_logging) {
        // If already logging, show a "Stop Logging" button
        if (ImGui::Button("Stop logging")) {
            m_simulation.stopLogging();
        }
    } else {
        // If not logging, show a "Start Logging" button
        if (ImGui::Button("Start logging")) {
            m_simulation.startLogging(m_gui_log_filename);
        }
    }

}

void Application::drawPlottingWindow()
{
    ImGui::Begin("Live statistics");

    ImGui::Checkbox("Auto-fit Data", &m_should_fit_plots);

    ImGui::Separator();

    // Get safe, read-only access to the raw data from the tracker (buffer)
    const auto& steps = m_simulation.getSteps();
    const auto& raw_magnetizations = m_simulation.getMagnetizations();
    const auto& raw_energies = m_simulation.getEnergies();

    // Normalistion factor
    int L = m_simulation.getLatticeDimension();
    double N = static_cast<double>(L * L);


    // Only try to plot if we have data
    if (!steps.empty()) {
        std::vector<double> norm_magnetizations;
        norm_magnetizations.reserve(raw_magnetizations.size());
        for (double val : raw_magnetizations) {
            norm_magnetizations.push_back(val / N);
        }

        std::vector<double> norm_energies;
        norm_energies.reserve(raw_energies.size());
        for (double val : raw_energies) {
            norm_energies.push_back(val / N);
        }

        // For the specific heat capacity
        const auto& raw_variances = m_simulation.getEnergyVariances();

        // Create a plot of magnetisation vs. time
        if (m_should_fit_plots) {
            ImPlot::SetNextAxesToFit();
        }
        if (ImPlot::BeginPlot("Magnetisation per spin vs. time")) {
            ImPlot::SetupAxes("Time step", "Total magnetisation/spin");
            ImPlot::PlotLine("M", steps.data(), norm_magnetizations.data(), static_cast<int>(steps.size()));
            ImPlot::EndPlot();
        }

        // Create a plot of energy vs. time
        if (m_should_fit_plots) {
            ImPlot::SetNextAxesToFit();
        }
        if (ImPlot::BeginPlot("Energy per spin vs. Time")) {
            ImPlot::SetupAxes("Time step", "Total energy/spin");
            ImPlot::PlotLine("E", steps.data(), norm_energies.data(), static_cast<int>(steps.size()));
            ImPlot::EndPlot();
        }

        if (!raw_variances.empty()) {
            double T = m_simulation.getTemperature();
            double kT2 = 1.0 * T * T; // Boltzmann constant k=1

            // Convert Variance(TotalEnergy) -> Specific Heat Capacity
            // Cv = (<E^2> - <E>^2) / (N * k * T^2)
            std::vector<double> specific_heat;
            specific_heat.reserve(raw_variances.size());

            for (double var : raw_variances) {
                specific_heat.push_back(var / (N * kT2));
            }

            if (m_should_fit_plots) {
                ImPlot::SetNextAxesToFit();
            }
            if (ImPlot::BeginPlot("Specific Heat")) {
                ImPlot::SetupAxes("Time step", "Cv (per spin)");
                ImPlot::PlotLine("Cv", steps.data(), specific_heat.data(), static_cast<int>(steps.size()));
                ImPlot::EndPlot();
            }
        }
    }
    ImGui::End();
}

int main(int argc, char* argv[])
{
    if (argc > 1) {
        std::string mode_arg = argv[1];

        if (mode_arg == "--experiment") {
            std::cout << "Experiment mode detected." << std::endl;

            // We expect at least one temperature value after the flag
            if (argc <= 2) {
                std::cerr << "Error: --experiment flag requires at least one temperature value." << std::endl;
                return 1;
            }

            std::vector<double> temperatures;
            for (int i = 2; i < argc; ++i) {
                try {
                    double temp = std::stod(argv[i]);
                    temperatures.push_back(temp);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Error: Invalid temperature value '" << argv[i] << "'. Please provide numbers only." << std::endl;
                    return 1;
                }
            }

            std::cout << "Temperatures to be simulated:" << std::endl;
            for (const auto& temp : temperatures) {
                std::cout << temp << std::endl;
            }

            ExperimentController experiment(temperatures);
            experiment.run();

            return 0; // Exit successfully after the experiment is done
        }
    }

    // Interactive GUI mode
    std::cout << "Interactive mode started." << std::endl;
    Application app;

    if (app.isInitialized()) {
        app.run();
    }

    return 0;

}
