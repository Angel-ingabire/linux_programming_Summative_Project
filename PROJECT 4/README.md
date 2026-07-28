# Project 4: Multithreaded Order Processing System

## Overview

Simulates an online food delivery order processing system using POSIX threads, mutex locks, and condition variables. The system has three threads:

1. **Kitchen Thread (Producer)** - Generates orders every 2 seconds
2. **Delivery Thread (Consumer)** - Processes deliveries every 4 seconds
3. **Monitoring Thread** - Reports system status every 5 seconds

## Files

- `order_processing.c` - Main source code
- `Makefile` - Build configuration
- `README.md` - This file

## Build Instructions

### Linux

```bash
cd "PROJECT 4"
make
# or manually:
gcc -pthread -o order_processing order_processing.c
```

### Run

```bash
# Default: process 20 orders
./order_processing

# Custom number of orders
./order_processing 10
./order_processing 50
```

## Sample Output

```
=== Online Food Delivery Order Processing System ===
Queue capacity: 5 orders
Orders to process: 20
Preparation time: 2s, Delivery time: 4s

[KITCHEN] Kitchen thread started. Preparing orders...
[DELIVERY] Delivery thread started. Waiting for orders...
[MONITOR] Monitoring thread started. Reporting every 5 seconds.
[KITCHEN] Order #1 prepared! Queue size: 1
[DELIVERY] Order #1 picked up for delivery. Queue size: 0
[KITCHEN] Order #2 prepared! Queue size: 1
[KITCHEN] Order #3 prepared! Queue size: 2
[DELIVERY] Order #2 delivered successfully!
...

=== [MONITOR] System Status Report ===
  Orders prepared:  5
  Orders delivered: 2
  Current queue size: 3/5
========================================
```

## Synchronization Design

### Mutex Protection

- Shared queue operations (enqueue/dequeue) protected by `pthread_mutex_t`
- Counter updates for prepared/delivered orders under mutex
- Active user tracking protected by mutex

### Condition Variables

- `cond_not_full`: Kitchen waits when queue is full; Delivery signals when space available
- `cond_not_empty`: Delivery waits when queue is empty; Kitchen signals when order ready

### Circular Buffer Queue

- Fixed-size array with O(1) enqueue/dequeue
- No dynamic memory allocation
- Automatic wrap-around using modulo arithmetic
