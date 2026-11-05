# Elevator

This project traces system calls, adds three system calls to Linux kernel 6.16.12, and implements
two kernel modules: a `/proc/timer` module and a pet elevator scheduler.

## Group Members
- **James Mosely**: [your FSU email]
## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: Create `empty.c` and `part1.c`, then verify five added system calls with `strace`.
- **Assigned to**: James Mosely

### Part 2: Timer Kernel Module
- **Responsibilities**: Implement `my_timer` and `/proc/timer`, reporting current and elapsed time.
- **Assigned to**: James Mosely

### Part 3a: Adding System Calls
- **Responsibilities**: Add wrappers and table entries for system calls 548 through 550.
- **Assigned to**: James Mosely

### Part 3b: Kernel Compilation
- **Responsibilities**: Configure, build, install, and verify kernel 6.16.12 in a VirtualBox VM.
- **Assigned to**: James Mosely

### Part 3c: Threads
- **Responsibilities**: Run the elevator in a kthread that stops cleanly on module removal.
- **Assigned to**: James Mosely

### Part 3d: Linked List
- **Responsibilities**: Store waiting pets and riders in `linux/list.h` lists using `kmalloc`.
- **Assigned to**: James Mosely

### Part 3e: Mutexes
- **Responsibilities**: Protect shared state across the system calls, kthread, and proc reads.
- **Assigned to**: James Mosely

### Part 3f: Scheduling Algorithm
- **Responsibilities**: Implement LOOK scheduling with FIFO boarding and capacity limits.
- **Assigned to**: James Mosely

## File Listing
```
elevator/
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   └── Makefile
├── part2/
│   ├── src/
│   │   └── my_timer.c
│   └── Makefile
├── part3/
│   ├── src/
│   │   ├── elevator_main.c
│   │   ├── elevator_proc.c
│   │   ├── elevator_thread.c
│   │   └── pet_elevator.h
│   ├── tests/
│   ├── Makefile
│   └── syscalls.c
├── Makefile
└── README.md

```
# How to Compile & Execute

### Requirements
- **Compiler**: `gcc`
- **Dependencies**: Ubuntu 24.04 (x86_64), `make`, `strace`, and Linux 6.16.12 with system calls 548 through 550 installed.

## Part 1

### Compilation
```bash
cd part1
make
```
This will build the `empty` and `part1` executables in `part1/`.
### Execution
```bash
make trace
```
This will run both programs under `strace` and write `empty.trace` and `part1.trace`.

## Part 2

### Compilation
```bash
cd part2
make
```
This will build `my_timer.ko` in `part2/`.
### Execution
```bash
sudo insmod my_timer.ko
cat /proc/timer
sleep 1
cat /proc/timer
sudo rmmod my_timer
```
This will load the module, print the current and elapsed times, and remove the module.


## Part 3

### Compilation
Install the system calls in Linux 6.16.12 once:
1. Copy `part3/syscalls.c` to `syscalls/syscalls.c` in the kernel source.
2. Create `syscalls/Makefile` containing `obj-y := syscalls.o`.
3. Add `syscalls/` to `core-y` in the top-level kernel `Makefile`.
4. Add entries 548 through 550 to `arch/x86/entry/syscalls/syscall_64.tbl`.
5. Add the three prototypes to `include/linux/syscalls.h`.
6. Disable `SYSTEM_REVOCATION_KEYS` and `SYSTEM_TRUSTED_KEYS`, then build and install the kernel.

Then build the module:
```bash
cd part3
make
```
This will build `elevator.ko` in `part3/`.
### Execution
```bash
sudo insmod elevator.ko
cd tests/elevator-test
make
./consumer --start
./producer 50
watch -n 1 cat /proc/elevator
./consumer --stop
sudo rmmod elevator
```
This will load the module, start the elevator, issue requests, and display its status. Wait for
`/proc/elevator` to show `OFFLINE` before removing the module.

## Development Log
Each member records their contributions here.

### James Mosely

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-11-04 | Created the GitHub repository. |
| 2026-11-05 | Added the starter files, completed Part 1, and set up the Ubuntu VM. |
| 2026-11-05 | Added the system calls, then built, installed, and tested kernel 6.16.12. |
| 2026-11-05 | Completed and tested the timer and elevator modules. |


## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| N/A        | James Mosely (solo)  | N/A              | No group meetings were held. |



## Bugs
- No known bugs.

## Considerations
- The elevator uses LOOK scheduling. It continues in its current direction while a rider or waiting pet needs service ahead, then reverses.
- At each floor, riders unload first. Waiting pets then board in FIFO order until the next pet would exceed 5 pets or 50 lbs.
- `producer` can generate floor 6. Requests with an invalid floor, an invalid pet type, or matching start and destination floors return 1.
- After a stop request, the elevator ignores waiting pets, delivers its riders, and goes `OFFLINE`. Waiting pets are serviced after a restart.