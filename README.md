# KataHex

KataHex is a Hex engine that is based on Katago. See the original [KataGo README](https://github.com/lightvector/KataGo#readme) for a general information on how it works.

You can find KataHex in the Hex2022 branch of this repository. It was
created by HZY (<2658628026@qq.com>). Peter Selinger
(<selinger@mathstat.dal.ca>) has made further modifications to make
KataHex more compatible with HexGUI.

## Results and conclusions

Some results and conclusions about opening move win rates are here:
<https://zhuanlan.zhihu.com/p/476464087>

## Building

The following works on Ubuntu 20.04, and probably in other Linuxes too:

If you don't have a GPU, make sure the Eigen library is installed:

    apt install libeigen3-dev

If you do have a GPU, you can use another one of the other GPU
backends below: `-DUSE_BACKEND=CUDA`, `-DUSE_BACKEND=TENSORRT`, or
`-DUSE_BACKEND=OPENCL`.

    mkdir build
    cd build
    cmake -DUSE_BACKEND=EIGEN ../cpp
    make -j4

By default, the maximum board size is 13x13. To specify a larger
maximum board size, add something like `-DMAX_BOARD_LEN=19` to the cmake
call.

For Windows, pre-built KataHex executables are available on the Releases page.

## Running

If you would just like to run KataHex (as opposed to training it
yourself), you need both a configuration file and a pre-trained neural
network model.

A sample configuration file is `config.cfg`, included in this
repository. Use it as it is, or modify it for your purposes.

You can find a pre-trained neural network model here:

[katahex_model_20220618.bin.gz](https://drive.google.com/file/d/1xMvP_75xgo0271nQbmlAJ40rvpKiFTgP/view)

This neural network can play 19x19 or smaller.

You can run KataHex like this, except using the path to your own
config file and model:

    katahex gtp -config config.cfg -model model.bin.gz

In this mode, KataHex speaks the [GTP
protocol](https://www.hexwiki.net/index.php/GTP). It's possible to
interact with it directly, although you may prefer to use a graphical
user interface such as [HexGUI](https://github.com/selinger/hexgui).

## Integration with HexGUI

For integration with [HexGUI](https://github.com/selinger/hexgui) or
other graphical frontends, it is most convenient to modify
katahex-launcher.sh with the correct paths to katahex, the
configuration file, and the model.  Then under HexGUI → Program →
New Program, add "KataHex" as the name and the path to
katahex-launcher.sh as the command.

After connecting KataHex to HexGUI, you can access KataHex analysis by
using the calculator icon on the main menu bar or pressing "s" (for
"solve").

Note that KataHex is often very slow; making a single move seems to
take several seconds. If the UI seems to be stuck for a long time,
just wait.
