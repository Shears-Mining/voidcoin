# VoidCoin Core

[![GitHub release](https://img.shields.io/github/v/release/VoidHash-Crypto/VoidCoin-Core)](https://github.com/VoidHash-Crypto/VoidCoin-Core/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

> **Preserved and maintained by [Shears Mining](https://sha256-mining.shears.co.uk).**
> The original repository (`VoidHash-Crypto/VoidCoin-Core`) is no longer online and the original developers
> could not be reached (if that's you, please get in touch). This repository keeps their complete history and release tags (v1.0.0 - v1.0.5) unchanged on
> the `main` branch. VoidCoin Core **v1.1.0** adds merge-mining (AuxPoW) with Bitcoin from block **56,500** -
> see [Merge-mining](#merge-mining-v110) below. All credit for VoidCoin itself goes to the VoidHash-Crypto team.
>
> VoidCoin is a fork of **Kvanta5-Core**: the quantum-resistant cryptography (ML-DSA-87 signatures and the
> P2QR address/script engine) and much of the consensus code are the **Kvanta5 team's** work, used under
> the MIT licence.
>
> **Block explorer:** https://void-explorer.shears.co.uk

**VoidCoin Core** is the reference implementation for the VoidCoin network — a proof-of-work blockchain built around native quantum-resistant ownership, designed for long-term monetary durability in a post-quantum world.

While most cryptocurrencies remain vulnerable to future quantum computing attacks against their underlying signature schemes, VoidCoin introduces **P2QR (Pay-to-Quantum-Resistant)** addresses natively at the protocol level, giving holders a path to quantum-safe custody without waiting for a hard fork.

---

## Table of Contents

- [Network Properties](#network-properties)
- [Address Types](#address-types)
- [Building VoidCoin Core](#building-voidcoin-core)
- [Running a Node](#running-a-node)
- [Mining](#mining)
- [Consensus Notes](#consensus-notes)
- [Community](#community)
- [License](#license)

---

## Network Properties

| Parameter | Value |
|---|---|
| Ticker | `VOID` |
| Algorithm | SHA256d (Proof of Work) |
| Max Supply | 231,000,000 VOID |
| Block Reward | 50 VOID |
| Halving Interval | Every 2,100,000 blocks |
| Block Target | 60 seconds |
| P2P Port | `7777` |
| RPC Port | `7778` |
| Premine | None |
| ICO | None |

The smallest unit of VOID is the **Quark**, where `1 VOID = 100,000,000 Quarks`.

---

## Address Types

The VoidCoin Core wallet GUI exposes two receive address formats, both backed by the same quantum-resistant key material:

| Prefix | Type | Description |
|---|---|---|
| `vqr1...` | P2QR | Native quantum-resistant address |
| `3...` | P2SH | Wrapped P2QR address, for mining pool / legacy software compatibility |

`vqr1...` addresses use a post-quantum signature scheme designed to remain secure even against an adversary with access to a cryptographically-relevant quantum computer — a property traditional ECDSA-based addresses (used by Bitcoin and most other chains) do not have.

Legacy P2PKH (`V...`) addresses remain supported at the protocol and RPC level for compatibility, but are intentionally not offered in the wallet's Receive tab. This is a deliberate design choice to steer users toward quantum-resistant addresses by default.

---

## Building VoidCoin Core

VoidCoin Core builds with the standard Bitcoin Core CMake toolchain.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DWITH_ZMQ=ON -DBUILD_GUI=OFF
cmake --build build -j$(nproc)
```

To build the Qt GUI wallet, set `-DBUILD_GUI=ON` instead. Detailed platform-specific build notes are available in the documentation directory:

- [Unix build notes](doc/build-unix.md)
- [Windows build notes](doc/build-windows.md)
- [macOS build notes](doc/build-osx.md)

Pre-built binaries are published on the [Releases](https://github.com/Shears-Mining/voidcoin/releases) page of this repository. (The original VoidHash-Crypto releases page no longer exists.)

---

## Running a Node

```bash
./voidcoind -daemon -addnode=46.7.7.113:7777
./voidcoin-cli getblockchaininfo
```

A minimal `voidcoin.conf`:

```ini
server=1
listen=1
port=7777
rpcport=7778
rpcuser=yourusername
rpcpassword=yourpassword
rpcallowip=127.0.0.1
addnode=46.7.7.113:7777
```

The data directory defaults to `~/.voidcoin` on Linux/macOS and `%APPDATA%\VoidCoin` on Windows.

---

## Mining

VoidCoin uses SHA256d, so it is compatible with any standard SHA256d mining software, including CGMiner, BFGMiner, and cpuminer-multi.
stratum+tcp://<pool-address>:<port>
Username: your VOID address
Password: x
Both legacy (`V...`/`3...`) and quantum-resistant (`vqr1...`) addresses can be used as the mining username, depending on pool support.

---

## Merge-mining (v1.1.0)

From block **56,500**, VOID can be **merge-mined with Bitcoin** (Namecoin-style AuxPoW, ported from
Namecoin Core). Bitcoin miners can mine VOID at the same time, using the same work, at no extra cost -
which brings far more hashrate to securing the chain.

| | |
|---|---|
| Activates at | block **56,500** (all nodes must run v1.1.0 or later by then) |
| Chain ID | **0x56** (86), strict |
| Normal (non-merge-mined) blocks | still valid for ever - direct miners carry on as before |
| Unchanged | 50 VOID reward, VCIP-2 200 VOID bounty, 0.1 VOID tail, 1-minute blocks, per-block difficulty |
| RPCs | `createauxblock <address>` / `submitauxblock <hash> <auxpow>` (and wallet `getauxblock`) |

Pool operators: see [doc/merge-mining.md](doc/merge-mining.md).

**Older nodes (v1.0.x) will stop following the chain at block 56,500.** Please upgrade before then.

---

## Consensus Notes

VoidCoin Core was forked from Kvanta5-Core. During initial mainnet bootstrap, the following consensus-affecting change was made prior to general availability:

### Removal of the block 1 dev fund allocation

The inherited codebase reserved block 1 as a special "dev fund" block, requiring its coinbase transaction to contain exactly 21 hardcoded outputs totaling 21,000,000 VOID, with normal SHA256d mining beginning only at block 2.

This allocation was removed before any blocks were mined on VoidCoin mainnet. As a result:

- Block 1 is a normal proof-of-work block, mined like every other block.
- The standard 50 VOID block subsidy applies starting from block 1, with the usual halving schedule applied from that point.
- There is no dev fund, no team allocation, and no pre-mined balance anywhere in VoidCoin's supply. All 231,000,000 VOID will be distributed exclusively through mining.

This change affects `src/validation.cpp` (block 1 coinbase validation and subsidy calculation) and `src/node/miner.cpp` (block template construction), and is reflected from the first commit that produced VoidCoin's genesis block onward. Anyone auditing the chain from genesis will see block 1 as an ordinary 50 VOID coinbase with no special-cased outputs.

---

## Community

- Block explorer: https://void-explorer.shears.co.uk
- Merge-mining pool: https://sha256-mining.shears.co.uk
- Discord: https://discord.gg/EmHXSJ4xHh
- GitHub: https://github.com/VoidHash-Crypto/VoidCoin-Core
- Organization: https://github.com/VoidHash-Crypto

---

## License

VoidCoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for details.
