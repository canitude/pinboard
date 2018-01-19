# Pinboard

Pinboard is a demo of P2P service implementing the metaphor of public notice board: anybody can pin a message to the board, anybody can view all pinned messages. Messages have a limited lifetime that depends on the numerical value of PoW hash computed over the concatenation of message hash, latest Litecoin block hash, and nonce: the smaller is that PoW hash value the longer is message life. To submit message user have to find (mine) nonce.

Pinboard uses Litecoin blockchain for timestamping and Litecoin P2P network for peer discovery. Pinboard nodes advertise themselves as a Litecoin full nodes with an additional non-standard service bit indicating Pinboard functionality.

Pinboard isn't ready for any practical use.

# Building

Make sure you have installed [libbitcoin](https://github.com/libbitcoin/libbitcoin) and [libaltcoin-network](https://github.com/canitude/libaltcoin-network) beforehand. In both [libbitcoin] and [libaltcoin-network] version3 branch is required.

```sh
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ../src/
$ make
```

# Launching

To launch a Pinboard P2P node type:

```sh
./pinboard
```

For other options use --help:

```sh
./pinboard --help
```

There is no UI.
