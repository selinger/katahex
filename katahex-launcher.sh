#!/bin/bash

# Modify these paths according to where you installed KataHex, its
# configuration file, and the neural network model.

BASEDIR="$HOME/projects/katahex"

KATAHEX="$BASEDIR/build/katahex"
CONFIG="$BASEDIR/config.cfg"
MODEL="$BASEDIR/katahex_model_20220618.bin.gz"

"$KATAHEX" gtp -config "$CONFIG" -model "$MODEL"
