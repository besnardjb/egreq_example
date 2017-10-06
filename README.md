# Extended Generic Request Example

## Dependencies

In order to use Extended Generic Requests you need an MPI runtime supporting
them. As they are not standard you may use (to my knowledge) either MPICH or one
of its derivative or the MPC MPI runtime.

Note that even if implemented there may be differences in the implementations
see for examples the IFDEFS in the code.

## How to Test

See the Makefile.

For MPICH:

```
make run
```


For MPC:

```
make runmpc
```


## Licence

This code is CECILL-C fully LGPL compatible feel free to use it if it is useful.
