# Project 5: Concurrent TCP Client-Server Monitoring System

## Overview

A university laboratory equipment booking system with a concurrent TCP server and multiple clients. Users can authenticate, view available equipment, and reserve items. The server handles multiple simultaneous clients using POSIX threads.

## Files

- `server.c` - Server application source code
- `client.c` - Client application source code
- `Makefile` - Build configuration
- `README.md` - This file

## Build Instructions

### Linux

```bash
cd "PROJECT 5"
make all
# or manually:
gcc -pthread -o server server.c
gcc -o client client.c
```

### Run Server

```bash
./server
```

### Run Client (in separate terminals)

```bash
./client 127.0.0.1 8080
```

## Registered Users

- student01, student02, student03
- researcher01, researcher02
- professor01

## Available Equipment

Spectrophotometer, Centrifuge, Microscope_A, Microscope_B, PCR_Thermocycler, Electrophoresis_Unit, pH_Meter, Balance_Analytical, Incubator_A, Incubator_B, Autoclave, Fume_Hood, Shaker_Incubator, Water_Bath, Vortex_Mixer, Magnetic_Stirrer, Spectrofluorometer, HPLC_System, Thermal_Cycler, Glove_Box

## Communication Protocol

### Text-based, newline-terminated messages

**Client -> Server:**
| Command | Description |
|---------|-------------|
| `LOGIN:<user_id>` | Authentication request |
| `LIST` | Request equipment list |
| `RESERVE:<equipment>` | Reservation request |
| `QUIT` | Session termination |

**Server -> Client:**
| Response | Description |
|----------|-------------|
| `AUTH_OK` | Authentication successful |
| `AUTH_FAIL` | Authentication failed |
| `EQUIPMENT:<list>` | Tab-separated equipment list with status |
| `RESERVED:<equipment>` | Reservation confirmed |
| `UNAVAILABLE:<equipment>` | Equipment already reserved |
| `ERROR:<message>` | Error notification |
| `BYE` | Session termination |

## Concurrency Design

### Thread-per-Client Model

- Each client gets a dedicated POSIX thread
- Independent execution contexts prevent blocking
- Threads are detached for automatic cleanup

### Shared Resource Protection

- `pthread_mutex_t` protects equipment records and active user list
- All read/write operations on shared data are within mutex lock/unlock
- Reservation requests are atomic (check availability + reserve under single lock)

### Error Handling

- Client disconnection releases all their reservations
- Invalid commands return proper error messages
- Server capacity limits prevent resource exhaustion
