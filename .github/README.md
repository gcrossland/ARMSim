# ARMSim

_[ Note: this was created in 2001 to consider what level of performance (in terms of instructions per cycle) a superscalar ARM implementation (along the lines of contemporaneous x86 implementations) might give; these days, you can simply run your code on one of the many concrete implementations. ]_

ARMSim is a processor simulator for the ARM (though it makes for very slow emulation). It simulates the basic internal units of a putative ARM processor and how they interact on a cycle-to-cycle basis. It can either simulate a traditional three-stage pipeline (as found in, say, the ARM610) or a five or six-stage superscalar version, with out-of-order execution; the quantity of execution units is configurable, so that users can study how the average number of instructions completed per cycle changes with different availability of computing resources. The output is an HTML page per cycle, showing the state of each functional unit once the cycle has completed.

_See [some example output from a three-stage in-order simulation](https://gcrossland.github.io/ARMSim/sm/s0000000.htm)._

_See [some example output from a five-stage out-of-order simulation](https://gcrossland.github.io/ARMSim/lg/s0000000.htm) (takes up quite a bit of screen real estate)._

## Licence

The content of the ARMSim repository is free software; you can redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl-2.0.txt) as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

The content is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

## Quick Start

*   Build by compiling all of the C source files both in the working directory (or archive) root and in libraries e.g. `gcc libraries/*.c *.c -o armsim`.
*   Run ARMSim with the supplied example ROM image (loaded at the bottom of the address space) and have it generate output from cycle 0 to 256 e.g. `mkdir outdir && ./armsim -r test.rom -s 0 -e 0x100 outdir`.
    *   The first cycle's output will be in outdir/s0000000.htm.

For further details, see [the manual](https://gcrossland.github.io/ARMSim/manual.html).
