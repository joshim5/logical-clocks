# logical-clocks
We are building models of asynchronous distributed systems to experiment on multiple implementations of logical clocks.

## Design Process
To understand our design decisions and follow our progress, please check our lab notebooks. We have two lab notebooks - one in the Python folder and one in the C folder. Temporally, the Python lab notebook was written before the C lab notebook.

## C Implementation Usage Information

The C implementation was tested under Mac OS X El Capitan. We did not have time
to test it on Ubuntu. It definitely does not work on Windows!

We are using `Apple LLVM version 7.0.2 (clang-700.1.81)` for compilation.

### Usage

To run, simply do:

`./coordinator.out`

and the resulting log files should be in the `log` folder. To compile, a simple
`Makefile` is provided.
