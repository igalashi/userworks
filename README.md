# NestDAQ Userworks

A collection of user-developed components for the NestDAQ data acquisition system, designed for high-energy physics experiments at J-PARC.

## What is NestDAQ?

NestDAQ is a modular data acquisition framework built on top of [FairMQ](https://github.com/FairRootGroup/FairMQ), providing scalable and flexible data processing pipelines for particle physics experiments. This userworks repository contains experiment-specific implementations and utilities.

## Components

### Core DAQ Components
- **Samplers**: Data acquisition devices for various detectors (HUL, SRS, DRS4, etc.)
- **Filters**: Real-time data filtering and processing (LogicFilter, TriggerView)
- **Builders**: Time frame construction and event building (TimeFrameBuilder, SubTimeFrameBuilder)
- **Sinks**: Data output and storage (FileSink, RecbeSink)
- **Displays**: Online monitoring and visualization tools

### Experiment-Specific Modules
- **e16**: Device programs for the E16 experiment
- **elph2018**: Components used for detector testing at ELPH in 2018
- **exam1-6**: Test and evaluation programs for various detector systems
- **recbe**: Readout programs for COMET CDC detector
- **utility**: Shared libraries and utilities from high-p beam line experiments

## Why Use NestDAQ Userworks?

- **Modular Architecture**: Easily extensible components for custom detector interfaces
- **High Performance**: Optimized for high-throughput data acquisition in real-time environments
- **Experiment Flexibility**: Supports multiple detector types and experimental configurations
- **Integration**: Seamless integration with CERN ROOT and other HEP analysis tools

## Getting Started

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+)
- CMake 3.11+
- [FairMQ](https://github.com/FairRootGroup/FairMQ)
- [Boost](https://www.boost.org/)
- [CERN ROOT](https://root.cern/) (optional, for analysis components)

### Building

1. Clone the repository:
```bash
git clone https://github.com/igalashi/nestdaq.git
cd nestdaq/src/userworks
```

2. Configure with CMake:
```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/nestdaq \
      -DCMAKE_PREFIX_PATH=$HOME/nestdaq:$HOME/root \
      -DCMAKE_CXX_STANDARD=17 \
      ..
```

3. Build and install:
```bash
make -j$(nproc)
make install
```

### Running Examples

The `elph2018` directory contains demonstration programs:

```bash
# Run a benchmark sampler
./bin/BenchmarkSampler --help

# Run a time frame builder
./bin/TimeFrameBuilder --help
```

For specific experiment setups, refer to the README files in individual subdirectories.

## Usage Examples

### Basic Data Flow
```
Sampler → Filter → TimeFrameBuilder → Sink
```

### Configuration
Most components accept configuration via command-line options or configuration files. Use `--help` to see available options:

```bash
./bin/TimeFrameBuilder --help
```

## Project Structure

```
userworks/
├── e16/           # E16 experiment components
├── elph2018/      # ELPH 2018 test components
├── exam1-6/       # Test and evaluation programs
├── recbe/         # COMET CDC readout
├── utility/       # Shared utilities
├── chkfile/       # Data validation tools
├── bin/           # Built executables
└── CMakeLists.txt # Build configuration
```

## Getting Help

- **Issues**: Report bugs and request features on [GitHub Issues](https://github.com/igalashi/nestdaq/issues)
- **Documentation**: Check individual component headers and example configurations
- **Community**: Contact the J-PARC HEP community for experiment-specific support

## Maintainers

- [Igalashi](https://github.com/igalashi) - Lead developer

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Submit a pull request with detailed description
4. Ensure code follows the existing style and includes appropriate tests

## License

This project is part of the NestDAQ framework. See the main repository for licensing information.