# Merge-mining VoidCoin (VOID) - for pool operators

VoidCoin Core v1.1.0 supports standard Namecoin-style auxiliary proof-of-work (AuxPoW) from block **56,500**.
Any pool that already merge-mines Namecoin, Doichain, Syscoin or similar can add VOID as one more aux chain.

## The essentials

| | |
|---|---|
| Algorithm | SHA-256d |
| Merge-mining start | block 56,500 (mainnet) |
| Chain ID | 0x56 (86) - strict: the parent chain must not use the same ID |
| Slot in a 32-slot merkle tree (nonce 0) | 28 |
| Block time | 60 seconds, per-block difficulty (Dark Gravity Wave) |
| Reward | 50 VOID, VCIP-2 bounty blocks 200 VOID, 0.1 VOID tail - computed by the node |
| Default ports | P2P 7777, RPC 7778 |

## RPC

```text
createauxblock <VOID address>   -> {"hash", "chainid": 86, "previousblockhash", "coinbasevalue",
                                    "bits", "height", "_target"}
submitauxblock <hash> <auxpow>   -> true if the block was accepted
getauxblock                      -> wallet-paid variant (needs the node's wallet)
```

`createauxblock` refuses before the start height ("mining auxblock method is not yet available").
The coinbase is built by the node and pays the address given to `createauxblock` in full - there is no
developer fee or required extra output.

Commit `hash` in the parent (Bitcoin) coinbase the usual way: `fabe6d6d` + merkle root of the aux
chains + tree size + nonce. When a share meets the VOID target, send the AuxPoW proof with
`submitauxblock`.

## Notes

- **Normal blocks stay valid after 56,500.** Only merge-mined blocks must carry chain ID 0x56; direct
  (stratum) miners can keep using full version rolling (ASICBoost).
- Merge-mined blocks before height 56,500 are rejected (`early-auxpow-block`).
- VOID addresses: `vqr1...` (quantum-resistant P2QR, recommended), `3...` (wrapped P2QR), or legacy `V...`.
  The VoidCoin wallet only hands out `vqr1`/`3` addresses by default.

## Example `voidcoin.conf`

```ini
server=1
listen=1
rpcbind=127.0.0.1
rpcallowip=127.0.0.1
rpcport=7778
rpcuser=CHANGE_ME
rpcpassword=CHANGE_ME_TOO
```

Questions: open an issue on this repository or ask on [questions.shears.co.uk](https://questions.shears.co.uk).
