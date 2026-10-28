# VoidCoin Core v2.1.0

> **Preserved and maintained by [Shears Mining](https://sha256-mining.shears.co.uk).**
> The VoidCoin network stopped at block **49,390** on 7 Aug 2026 after the original developers stepped
> away. That block's hash (`000000000000105fb377c3ce7c919d406a4a2530fd4abb08cd00e2352f8627d1`) is identical
> on the surviving 2.0.3 node of the NestEx exchange and on this chain, which continues from it - so every
> balance from the original network is preserved. All credit for VoidCoin itself goes to the VoidCoin
> Development Team (VoidHash-Crypto). If that's you, please get in touch.
>
> **v2.1.0 = the original VoidCoin Core v2.0.3 + merge-mining (AuxPoW) with Bitcoin from block 56,500.**
> Nothing else in the consensus rules changes: VCIP-3 (active since block 32,000) and all 2.0.3 rules are
> kept exactly. Wallets made by 2.0.3 open in 2.1.0.
>
> **Vault pages are not included.** The Kvanta5 Vault pages in the 2.0.3 graphical wallet are proprietary
> Kvanta5 code, licensed for v2.0.3 only, so they have been removed from this release. The consensus rules
> for P2QR and vault outputs are unchanged, so all vault coins remain valid on chain, and the vault RPCs
> (`createp2qrmultisig`, `dumpvoidcoinvaultbackup`, `importvoidcoinvaultbackup`) are still available.
> If you hold coins in a multi-signature vault and want to spend them with the graphical wallet, use the
> original v2.0.3 wallet **before block 56,500**.
>
> **Block explorer:** https://void-explorer.shears.co.uk

## Credits

- **VoidCoin Development Team (VoidHash-Crypto)** - VoidCoin and VoidCoin Core up to v2.0.3, including
  VCIP-1, VCIP-2 and VCIP-3. See [COPYRIGHT](COPYRIGHT).
- **Kvanta5 Organization** - VoidCoin is a fork of **Kvanta5-Core**. The quantum-resistant cryptography
  (ML-DSA-87 signatures and the P2QR address/script engine), the Vault pages (not included in this
  release) and the multisignature functionality are the Kvanta5 team's work. Original Kvanta5 implementations remain copyrighted by their
  authors; see [ATTRIBUTIONS.md](ATTRIBUTIONS.md) and the official Kvanta5 repository.
- **KvantaMechanic (Kvanta5)** - recovered and published the v2.0.3 source this release is built on
  (https://github.com/KvantaMechanic/VoidCoin, preserved at Shears-Mining/voidcoin-2.0.3), with corrected
  copyright attribution.
- **Namecoin Core developers** - the AuxPoW (merge-mining) code, ported from Namecoin Core nc29.1.
- **Bitcoin Core developers** - the base node software.

## Merge-mining (new in v2.1.0)

From block **56,500**, VOID can be **merge-mined with Bitcoin** (Namecoin-style AuxPoW). Bitcoin miners
can mine VOID at the same time, with the same work, at no extra cost.

| | |
|---|---|
| Activates at | block **56,500** - every node must run v2.1.0 or later by then |
| Chain ID | **0x56** (86), strict |
| Normal (non-merge-mined) blocks | stay valid for ever - direct miners carry on as before |
| Unchanged | all v2.0.3 consensus rules, rewards and 1-minute blocks |
| RPCs | `createauxblock <address>` / `submitauxblock <hash> <auxpow>` (and wallet `getauxblock`) |

Pool operators: see [doc/merge-mining.md](doc/merge-mining.md).

**Nodes running v2.0.3 or older will stop following the chain at block 56,500. Please upgrade before then.**

## Building

```
cmake -B build
cmake --build build -j$(nproc)
```

See [doc/build-unix.md](doc/build-unix.md) and [doc/build-windows.md](doc/build-windows.md).

## Licence

VoidCoin Core is released under the MIT licence - see [COPYING](COPYING). Trademarks and original
contributions are listed in [COPYRIGHT](COPYRIGHT); third-party credits in [ATTRIBUTIONS.md](ATTRIBUTIONS.md).

---

## Note from the recovered v2.0.3 source (KvantaMechanic)

The v2.0.3 source this release builds on was recovered and published by KvantaMechanic. His notes,
kept as he wrote them:

> - **This is NOT a new cryptocurrency or chain launch.**
> - v2.0.3 is a later release than the commonly preserved v1.0.5.
> - Earlier versions may follow incompatible blockchain histories.
> - Verify historical block hashes and consensus rules against surviving nodes before declaring any
>   recovered chain canonical.
> - Existing wallet files and historical balances should be preserved.
> - Do not assume that successfully syncing from genesis means you've recovered the correct chain.
>
> VoidCoin incorporates code originally developed by the **Kvanta5 Organization**, including
> implementations related to Pay-to-Quantum-Resistant (P2QR), Vault Pages and MultiSignature
> functionality. Original Kvanta5 implementations remain copyrighted by their respective authors.
>
> No claims of ownership over independently developed VoidCoin components are made by the Kvanta5
> Organization.
