# OFDM Link-Level Simulator in C++

This is a modular OFDM simulator written in modern C++. It goes through the
full chain — bits, QAM modulation, OFDM (IFFT + cyclic prefix), an AWGN
channel, then the receiver side (FFT, cyclic prefix removal, QAM
demodulation) — and checks the results against the known theoretical BER
curves.

Chain:

```
Bit Generation → QAM Modulation → OFDM Modulation (IFFT + CP)
→ AWGN Channel → OFDM Demodulation (FFT + CP removal)
→ QAM Demodulation → BER Analysis
```

What it currently does:

- QPSK, 16-QAM, 64-QAM
- AWGN channel
- Monte Carlo BER simulation
- Comparison against theoretical BER
- Cyclic prefix insertion/removal
- Unit tests for each stage

Each part of the system is its own class, so I can test and change pieces
without touching the rest.

![BER curve](results/ber_curve.png)

---

## Architecture

```
BitSource
    ↓
QAM Modulator
    ↓
OFDM Modulator
    ├── Subcarrier Mapping
    ├── IFFT
    └── Cyclic Prefix
    ↓
Channel (AWGN)
    ↓
OFDM Demodulator
    ├── Cyclic Prefix Removal
    ├── FFT
    └── Subcarrier Extraction
    ↓
QAM Demodulator
    ↓
BER Analysis
```

`LinkSimulator` wires all of this together and runs the Monte Carlo loop —
it doesn't do any signal processing itself, just calls the other classes in
order and keeps count of bit errors.

---

## Files

```
    SystemConfig.hpp     - all the OFDM/simulation parameters in one place
    BitSource.hpp         - random bit generator
    QamModulator.hpp      - QPSK/16-QAM/64-QAM modulation and demodulation
    OfdmModulator.hpp     - subcarrier mapping, IFFT/FFT, CP add/remove
    Channel.hpp            - AWGN
    BerAnalyzer.hpp       - theoretical BER formula, for comparison
    LinkSimulator.hpp     - runs the pipeline end to end

    LinkSimulator.cpp
    main.cpp              - runs one modulation scheme
    main_all_mod.cpp      - runs QPSK, 16-QAM, 64-QAM together

    plot_ber.py            - plots the BER results

    kissfft.hh             - the FFT library (single header)
```

---

## Current configuration

```
FFT size              = 64
Used subcarriers      = 52
Cyclic prefix length  = 16
Modulation            = QPSK (can switch to 16-QAM or 64-QAM)
SNR range             = 0 to 20 dB
```

- QPSK — 2 bits per symbol
- 16-QAM — 4 bits per symbol
- 64-QAM — 6 bits per symbol

---

## How each part works

### Bit generation

`BitSource` produces random bits using `std::mt19937` with a fixed seed, so
runs are repeatable.

### QAM modulation

`QamModulator` turns groups of bits into complex symbols (Gray-coded square
QAM), and the same class demodulates them back with a hard-decision rule.
Symbol energy is normalized to 1, which keeps the Eb/N0 math simple and
consistent across QPSK/16-QAM/64-QAM.

### OFDM modulation

`OfdmModulator` takes the QAM symbols, maps them onto subcarriers (around
DC, DC itself left empty), runs a 64-point IFFT, and adds the cyclic
prefix — the last 16 samples of the symbol, copied to the front:

```
symbol:      x[0] x[1] ... x[47] x[48] ... x[63]
CP:                                x[48] ... x[63]
transmitted: x[48] ... x[63]  x[0] x[1] ... x[63]
```

### OFDM demodulation

Same class, reverse order: strip the CP, run the FFT, pull the data back
off the subcarriers.

The IFFT/FFT both use 1/sqrt(N) scaling, so signal energy stays consistent
between time and frequency domain — this matters for getting the noise
level right later.

### Channel

Right now it's AWGN only:

```
y[n] = x[n] + w[n]
```

Noise is added to the time-domain waveform, before the CP is removed —
same order a real receiver would see it in.

### BER analysis

```
BER = bit errors / total bits transmitted
```

Compared against the standard theoretical formula for Gray-coded square
QAM in AWGN:

```
BER ≈ (4/k)(1 - 1/sqrt(M)) × Q(sqrt(3k/(M-1) × Eb/N0))
```

where M is the modulation order and k = log2(M).

---

## Results

Ran the Monte Carlo sweep for QPSK, 16-QAM, and 64-QAM. Simulated BER
tracks the theoretical curve closely across all three. The small gap you
see for 64-QAM at low SNR is expected — the theoretical formula is an
approximation that gets less accurate for higher-order QAM at low SNR, not
a bug in the simulation.

---

## Cyclic prefix

With FFT size 64 and CP length 16, each OFDM symbol is 80 samples total.
The CP carries no new information — it's just a copy of the tail of the
symbol, there to absorb multipath delay spread and stop symbols from
interfering with each other.

A proper CP study is planned for the next phase (see below).

---

## Why it's built this way

**Separate classes.** Each block (bits, QAM, OFDM, channel, BER) is its
own class so I can swap one out without touching the others — e.g. later
I can drop in Rayleigh fading in `Channel` without changing anything in
`QamModulator`.

**Unitary FFT/IFFT (1/sqrt(N)).** Keeps energy consistent across the
transform, which makes the noise/Eb-N0 calculations straightforward.

**Noise added in the time domain.** Matches where noise actually enters a
real receiver — before CP removal and the FFT — rather than being added
directly to the QAM symbols.

**Symbol energy normalized to 1.** Gives a common reference point across
QPSK/16-QAM/64-QAM so Eb/N0 means the same thing for all of them.

---
## Tests
- **Bit generation test** — Generates a desired count of bit sequence
- **QAM test** — modulation, demodulation, round-trip, energy check
- **OFDM test** — IFFT/FFT, CP add/remove, energy preserved through the
  transform, round-trip
---

## Next phase

**1. Channel coding.** Adding a Hamming encoder/decoder around the
existing chain:

```
Bits → Hamming Encoder → QAM → OFDM → Channel → OFDM Demod
→ QAM Demod → Hamming Decoder → BER
```

Looking at coded vs. uncoded BER, how many errors get corrected, and the
data-rate cost of the coding overhead.

**2. Rayleigh multipath fading.** Replacing the AWGN-only channel with a
multipath model (several delayed, faded paths). Looking at frequency-
selective fading, BER vs. AWGN, and how the CP holds up once there's real
multipath delay spread to deal with.

**3. A proper cyclic prefix study.** What happens when CP length is
shorter than the channel delay spread (ISI, broken orthogonality) vs. long
enough (clean circular convolution, easy frequency-domain equalization),
and the overhead/data-rate trade-off of making the CP longer.

## Later on

Channel estimation, ZF/MMSE equalization, DMRS/pilot insertion,
constellation plots after the channel, BLER, PAPR analysis, more channel
coding options.