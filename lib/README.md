# Tools to collect Hardware Counters and Instrumentation Metrics

It requires several step to use this tool.

1. Building the tools.
2. Calling counting library around application region of interest
3. Running the application with several policies to gather runtime and counters.
4. Running the application with pin to gather instrumentation metrics.
5. Stitch the results in one file.

## Requirements

Install these in your standard path or edit counters/Makefile, instrumentation/Makefile.inc
to setup link and include paths to these libraries location.

* hwloc >= 2.0
* papi

## Setup

### Build Hardware Counters Library

```
make -C counters
cp counters/libcounters.so /usr/lib
cp counters/counting.h /usr/include
```

### Build Pin tool

Edit `PIN_ROOT` variable instrumentation/Makefile.inc to point
to pin binary, then
```make -C instrumentation```

## Instrument Your Application

In your application:

```
#include <counting.h>

counting_set_output("my_output.csv");
counting_set_info_field("My app name");

struct eventset * evset = counting_init(getpid());
...

counting_start(evset);
// Region of interest here.
counting_stop(evset);

...
counting_stop(evset);

```

Link at compilation with flag `-lcounters`

## Run and Acquire Metric.

### Hardware Counters

Each run of your application with generate a file "my_output.csv" containing
hardware counters values and timer value.

### Instrumentation Metrics

run:
```pin -t src/obj-intel64/PinTool.so -- prog <args>```

The affinity matrices for region of interest are output in '.mat' files.
The instrumentation metrics summary is output in "metrics.csv" file.

## Debugging Pin Tool

* Compile pintool with debug options
Uncomment `DEBUG` in `instrumentation/Makefile.inc`, then recompile.

* Run in debug mode:
```
$PIN_ROOT/pin -pause_tool 2 -t src/obj-intel64/PinTool.so -- prog <args> 1>pin.out &
sleep 1 && cat pin.out
pid=$(awk /pid/'{print $11}' pin.out)
cmd=$(awk /add/'{print}' pin.out)
gdb --quiet --eval-command="$cmd" --pid $pid $PIN_ROOT/intel64/bin/pinbin    
```
