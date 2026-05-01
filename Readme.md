# SAM — Physical-layer Algorithm Library & Model

Xcelerium framework for building high-fidelity algorithmic reference
models of communication systems (4G LTE / 5G NR). Intended as a floating and fixed-point
implementation reference targeting SDR platforms.

---

## Repository Layout

```text
./
├── Readme.md
├── lib/
│   ├── Makefile
│   ├── bin/
│   │   └── sam/
│   │       ├── libsam.a
│   │       └── obj/
│   │           ├── rx/
│   │           ├── tx/
│   │           └── utils/
│   ├── include/
│   │   └── sam/
│   │       ├── core/
│   │       │   ├── base.h
│   │       │   ├── constants.h
│   │       │   ├── fx_utils.h
│   │       │   └── signal_types.h
│   │       ├── rx/
│   │       │   └── ofdm_demod.h
│   │       ├── tx/
│   │       │   └── ofdm_mod.h
│   │       └── utils/
│   │           ├── argsparser.h
│   │           ├── lte_pdsch_params.h
│   │           └── params_reader.h
│   └── src/
│       ├── rx/
│       │   └── ofdm_demod.cpp
│       ├── tx/
│       │   └── ofdm_mod.cpp
│       └── utils/
│           ├── argsparser.cpp
│           ├── lte_pdsch_params.cpp
│           └── params_reader.cpp
├── matlab/
├── tests/
│   ├── cpp_ref/
│   │   └── ofdm_loopback/
│   │       ├── Makefile
│   │       ├── bin/
│   │       │   └── test.exe
│   │       └── test/
│   │           └── test.cpp
│   ├── matlab_ref/
│   │   └── demod/
│   │       ├── Makefile
│   │       ├── bin/
│   │       │   └── test.exe
│   │       └── test/
│   │           └── test.cpp
│   └── misc/
│       ├── sanity_argsparser/
│       │   ├── Makefile
│       │   ├── bin/
│       │       │   └── test.exe
│       │       └── test/
│       │           └── test.cpp
│       └── sanity_lte_pdsch_params/
│           ├── Makefile
│           ├── params.txt
│           ├── bin/
│           │   └── test.exe
│           └── test/
│           └── test.cpp
```

## Build

### Prerequisites

* `g++` with C++17 support
* `pkg-config`
* `itpp` development headers and library

### Library

```bash
cd lib/

# debug build (default)
make

# release build
make BUILD=release

# clean build artifacts
make clean

# inspect resolved flags
make info
```

The library output is:

```text
lib/bin/sam/libsam.a
```

### Tests

```bash
# Build and run OFDM loopback test
cd tests/cpp_ref/ofdm_loopback/
make
./bin/test.exe

# Build and run demod test
cd tests/matlab_ref/demod/
make
./bin/test.exe --mode=lte --channel=pdsch --bandwidth=20

# Build and run argsparser sanity test
cd tests/misc/sanity_argsparser/
make
./bin/test.exe --mode=5g --channel=pbch --bandwidth=10
```

---

## Usage

### Running Demodulation Tests

The demodulation test (`tests/matlab_ref/demod/`) validates the OFDM demodulation against MATLAB reference vectors. It requires test vector files in a specific directory structure.

#### Command-line Arguments for Demod

The demod test accepts the following arguments:

* `--mode`: Communication standard (`lte` or `5g`)
* `--channel`: Channel type (`pdsch`, `pbch`, or `pdcch`)
* `--bandwidth`: (for lte) Bandwidth in MHz (`3`, `5`, `10`, `15`, or `20`)
* `--dmrs_mode`: (for 5g) Dmrs mode (`1`, `2`, `3`, `4`, or `5`)

Example usage:

```bash
./bin/test.exe --mode=lte --channel=pdsch --bandwidth=20
```

This will look for test vectors in `../test_vectors/lte/pdsch/20MHz/` relative to the test directory.

---

## Design Principles

| Principle | Description |
| :--- | :--- |
| **Algorithmic clarity** | Code mirrors the mathematical intent directly. |
| **Explicit execution** | `eval()` is called explicitly by the orchestrator. |
| **Zero-copy** | Signal buffers are passed as non-owning views where possible. |
| **State encapsulation** | Persistent state is private with trailing underscore (`state_`). |
| **Determinism** | `reset()` restores initial conditions to match a fresh object. |
| **Composability** | Blocks are composed by sequential `eval()` calls. |

---

## Execution Model

Each block exposes two methods:

```cpp
void eval(const Inputs&, Outputs&, const Control&, const Config&, const ExecContext&);
void reset();
```

`eval()` processes one natural execution unit (for example, one OFDM symbol in `OFDMMod`). `reset()` clears internal state.

Higher-level orchestrators call child `eval()` methods in sequence:

```cpp
void PhyTx::eval(...) {
    back_end_.eval(...);
    mapper_.eval(...);
    ofdm_mod_.eval(...);
    duc_.eval(...);
}
```

---

## Implemented Blocks

### TX

| Block | Unit | Description |
| :--- | :--- | :--- |
| `sam::tx::OFDMMod` | OFDM symbol | IFFT, cyclic prefix addition, optional windowing, optional DFT precoding |

### RX

| Block | Unit | Description |
| :--- | :--- | :--- |
| `sam::rx::OFDMDemod` | OFDM symbol | FFT, cyclic prefix removal, subcarrier extraction, optional IDFT precoding |

---

## Data Representation

* Real signal data uses `double`
* Complex signal data uses `std::complex<double>` via `itpp::cvec`
* Fixed-point effects are modelled explicitly in `fx_utils.h`, not by the numeric type
