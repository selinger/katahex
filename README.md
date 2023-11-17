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
