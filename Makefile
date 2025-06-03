# Detect platform
OS := $(shell uname -s)

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude

# Default linker flags
LDFLAGS = -lcurl

# Additional Windows-specific libraries
ifeq ($(OS), MINGW64_NT-10.0)
	EXE = .exe
	LDFLAGS += -lssl -lcrypto -lz -lws2_32 -lcrypt32 -luser32 -lwldap32
else
	EXE =
endif

# Sources
SRC_EN = src/friday.cpp
SRC_ES = src/viernes.cpp

# Binaries
BIN_EN = bin/friday$(EXE)
BIN_ES = bin/viernes$(EXE)

# Install paths (Linux only)
INSTALL_PATH_EN = /usr/local/bin/friday
INSTALL_PATH_ES = /usr/local/bin/viernes

# Default target
all: $(BIN_EN) $(BIN_ES)

# English binary
$(BIN_EN): $(SRC_EN)
	$(CXX) $(CXXFLAGS) -o $(BIN_EN) $(SRC_EN) $(LDFLAGS)

# Spanish binary
$(BIN_ES): $(SRC_ES)
	$(CXX) $(CXXFLAGS) -o $(BIN_ES) $(SRC_ES) $(LDFLAGS)

# Build only (both versions)
build: $(BIN_EN) $(BIN_ES)
	@echo "Build complete: $(BIN_EN) and $(BIN_ES)"

# Install (Linux only)
install: $(BIN_EN)
ifeq ($(OS), Linux)
	install -m 755 $(BIN_EN) $(INSTALL_PATH_EN)
	@echo "Installed to $(INSTALL_PATH_EN)"
else
	@echo "Install not supported on this platform"
endif

# Uninstall (Linux only)
uninstall:
ifeq ($(OS), Linux)
	rm -f $(INSTALL_PATH_EN)
	@echo "Uninstalled from $(INSTALL_PATH_EN)"
else
	@echo "Uninstall not supported on this platform"
endif

# Instalar (Spanish build - Linux only)
instalar: $(BIN_ES)
ifeq ($(OS), Linux)
	install -m 755 $(BIN_ES) $(INSTALL_PATH_ES)
	@echo "Instalado en $(INSTALL_PATH_ES)"
else
	@echo "Instalación no soportada en esta plataforma"
endif

# Desinstalar (Spanish build - Linux only)
desinstalar:
ifeq ($(OS), Linux)
	rm -f $(INSTALL_PATH_ES)
	@echo "Desinstalado de $(INSTALL_PATH_ES)"
else
	@echo "Desinstalación no soportada en esta plataforma"
endif

# Clean all builds
clean:
	rm -f $(BIN_EN) $(BIN_ES)
