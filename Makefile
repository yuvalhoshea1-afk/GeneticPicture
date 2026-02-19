# ==================================================
# Makefile for GeneticPicture project
# ==================================================

TARGET = genetic_art

# Sources
SRCS = main.c image.c render.c genetic.c
OBJS = $(SRCS:.c=.o)

# Detect compiler (try gcc-15, then gcc, then clang)
ifeq (, $(shell command -v gcc-15 2>/dev/null))
  ifeq (, $(shell command -v gcc 2>/dev/null))
    ifeq (, $(shell command -v clang 2>/dev/null))
      $(error No compiler found! Install GCC or Clang.)
    else
      CC = clang
    endif
  else
    CC = gcc
  endif
else
  CC = gcc-15
endif

# OpenMP: optional. Disable with: make NO_OMP=1
ifndef NO_OMP
  OPENMP_TEST := $(shell echo | $(CC) -fopenmp -dM -E - 2>/dev/null | grep _OPENMP)
  ifneq ($(OPENMP_TEST),)
    OMP_CFLAGS  = -fopenmp
    OMP_LDFLAGS = -fopenmp
    $(info OpenMP: enabled)
  else
    $(info OpenMP: not supported by $(CC), building single-threaded)
  endif
else
  $(info OpenMP: disabled by NO_OMP=1)
endif

# SDL2 flags
SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL2_LIBS   := $(shell pkg-config --libs sdl2 2>/dev/null)

# Optimization flags
CFLAGS  = -Wall -Wextra -O3 -ffast-math -funroll-loops $(OMP_CFLAGS) $(SDL2_CFLAGS)
LDFLAGS = $(OMP_LDFLAGS) $(SDL2_LIBS) -lm

# ==================================================
# Build rules
# ==================================================
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run