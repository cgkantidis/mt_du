`mt_du` is a multi-threaded version of `du`, the Linux tool that shows you the Disk Usage of a set of directories/files
recursively.

Currently the options supported are:

```none
Usage: mt_du [--help] [--version] [--human-readable] [--threads VAR] paths

Positional arguments:
  paths                 the list of paths for which to print their disk size [nargs: 1 or more]

Optional arguments:
  -h, --help            shows help message and exits
  -v, --version         prints version information and exits
  -H, --human-readable  display the sizes in a human-readable format
  -j, --threads         number of threads to use [nargs=0..1] [default: <nproc>]
```

More options of `du` are going to be supported in the future.
