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
│   │       └── libsam.a
│   ├── include/
│   │   └── sam/
│   │       ├── core/
│   │       │   ├── base.h
│   │       │   ├── fx_utils.h
│   │       │   └── signal_types.h
│   │       ├── rx/
│   │       │   └── ofdm_demod.h
│   │       └── tx/
│   │           └── ofdm_mod.h
│   └── src/
│       ├── rx/
│       │   └── ofdm_demod.cpp
│       └── tx/
│           └── ofdm_mod.cpp
└── tests/
    └── ofdm_loopback/
        ├── Makefile
        ├── bin/
        │   └── test.exe
        └── test/
            └── test.cpp
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
cd tests/ofdm_loopback/
make
./bin/test.exe
```

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
