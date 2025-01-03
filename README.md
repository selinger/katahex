# KataHex 2024

KataHex is a Hex engine that is based on KataGo. See the original [KataGo README](https://github.com/lightvector/KataGo#readme) for a general information on how it works.

You can find KataHex in the Hex2024 branch of this repository. It was
created by HZY (<2658628026@qq.com>). Peter Selinger
(<selinger@mathstat.dal.ca>) has made further modifications to make
KataHex more compatible with HexGUI.

## Building

The following works on Ubuntu 20.04, and probably in other Linuxes too:

If you don't have a GPU, make sure the Eigen library is installed:

    apt install libeigen3-dev

If you do have a GPU, you can use another one of the other GPU
backends below: `-DUSE_BACKEND=CUDA`, `-DUSE_BACKEND=TENSORRT`, or
`-DUSE_BACKEND=OPENCL`.

    git clone https://github.com/selinger/katahex.git
    cd katahex
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

[hex27x3.bin.gz](https://drive.usercontent.google.com/download?id=1YeqRvAYs7YjtPh0xBbDnHxeo2xrLEOdX)

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


# KataHex 2024
Latest release: https://github.com/hzyhhzy/KataGo/releases/tag/Hex_20240908   
~300 elo stronger than 2022 version on 19x19 board   
New feature: Move number limitation    
## Results
The length of the optimal Hex game on n\*n board is about **0.45\*n^2** according to KataHex     
https://dev.to/hzyhhzy/analysis-of-the-length-of-optimal-games-of-hex-game-using-alphazero-like-ai-16n7   
Some conjectures of "Template problems" of Hex    
https://mathoverflow.net/questions/470376/connection-properties-of-a-single-stone-on-an-infinite-hex-board


## Training schedule
Scripts and parameters used in this run are in **./scripts**   
   
I will use **"RTX4090\*month"** or **"RTX4090\*day"** as the unit of training cost.    
Total: **~6 RTX4090\*month**       
    

The major part of this run is on 15x15 board to save money. **(~4 RTX4090\*month)**   
The cost of every selfplay game on n\*n board is **O(n^4)** (CNN cost \* move number). Consider that larger boards usually need more search visits per move, it can even reach **O(n^7)**.    
At the end it was trained on 19x19.**(~1 RTX4090\*month)**   
And finally 27x27 **(~3 RTX4090\*day)**    
    
By experience, if the largest board the model trained is n \* n, the model will play well on boards < 1.5n \* 1.5n.    
For example, for a model trained only on 15x15 and smaller boards, it plays well on 19x19 but not 27x27. After a short training on 19x19, it plays well on 27x27. At the end I trained it on 27x27, so it can barely play on a 41x41 board.
    
Later I implemented **"Move number limitation"** and continued training with this.   
On 15x15: **~4 RTX4090\*day**   
On 19x19: **~7 RTX4090\*day**   
On 27x27: **~4 RTX4090\*day** but failed. The winrate becomes weird and the reason is still unknown.
