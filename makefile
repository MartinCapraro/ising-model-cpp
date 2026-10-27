# BUILD CONFIGURATION
# Set to 1 to build with Modern OpenGL, set to 0 to build with Legacy OpenGL
MODERN_GL_ENABLED = 1

CXX = clang++
# CXXFLAGS = -std=c++17 -Wall -Wextra -g # to build with debugging flags
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
CXXFLAGS += -Wuninitialized -Wshadow -Wformat -Wconversion -Wdeprecated-declarations
CXXFLAGS += -Ivendor/imgui
CXXFLAGS += -Ivendor/implot

ifeq ($(MODERN_GL_ENABLED),1)
	CXXFLAGS += -DMODERN_GL
endif

ALL_PKGS = sdl2 glew glm
CXXFLAGS += $(shell pkg-config --cflags $(ALL_PKGS))
LDFLAGS += $(shell pkg-config --libs $(ALL_PKGS))

# Platform-Specific Flags
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -framework OpenGL
else ifeq ($(UNAME_S),Linux)
	LDFLAGS += -lGL
endif

# Target executable
TARGET = ising

BASE_SRC = main.cpp \
		IsingSimulation.cpp \
		ExperimentController.cpp \
      	globals.cpp \
      	random.cpp \
      	StatisticsTracker.cpp \
       	vendor/imgui/imgui.cpp \
	    vendor/imgui/imgui_draw.cpp \
	    vendor/imgui/imgui_tables.cpp \
	    vendor/imgui/imgui_widgets.cpp \
	    vendor/imgui/imgui_impl_sdl2.cpp \
	    vendor/implot/implot.cpp \
	    vendor/implot/implot_items.cpp


# Conditionally add the correct ImGui OpenGL backend
ifeq ($(MODERN_GL_ENABLED),1)
    BASE_SRC += vendor/imgui/imgui_impl_opengl3.cpp
else
    BASE_SRC += vendor/imgui/imgui_impl_opengl2.cpp
endif

ifeq ($(MODERN_GL_ENABLED),1)
	SRC = $(BASE_SRC) rendering/ModernGLRenderer.cpp shader_utils.cpp
else
	SRC = $(BASE_SRC) rendering/LegacyGLRenderer.cpp
endif

OBJS = $(SRC:.cpp=.o)


# Default rule: compile program
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

	# Remove the .o files after linking the executable
	rm -f $(OBJS)

	# Run the executable after successful compilation
	./$(TARGET)

# Compile individual .cpp files into .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)
