# VCIP-2: Quantum Uncertainty Difficulty (QUD) with Void Collapse Bounty Block

| Field | Value |
|---|---|
| VCIP | 2 |
| Title | Quantum Uncertainty Difficulty (QUD) — VoidCoin's Original Difficulty Algorithm |
| Author | VoidCoin Development Team |
| Status | Draft |
| Created | 2026-07-04 |
| Activation | Block 6,000 (mainnet) |
| Replaces | Dark Gravity Wave (DGW) |

---

## Abstract

This proposal introduces **Quantum Uncertainty Difficulty (QUD)**, VoidCoin's original proof-of-work difficulty adjustment algorithm, replacing the inherited Dark Gravity Wave (DGW) algorithm at block 6,000. QUD is the most attack-resistant SHA256d difficulty algorithm ever deployed on a proof-of-work blockchain, combining seven original security mechanisms under a unified quantum-inspired framework.

This proposal also introduces the **Void Collapse Bounty Block** — a consensus-layer random reward mechanism — and the **Tail Emission Floor**, ensuring permanent chain security after the final halving.

No other proof-of-work blockchain implements any combination of these features. All mechanisms described in this document are original contributions by the VoidCoin Development Team, timestamped on GitHub on 2026-07-04.

---

## Motivation

On 2026-07-04, VoidCoin experienced a sustained 51% hashrate attack between blocks 5,587 and 5,715. A single entity directed approximately 102 petahashes/second of SHA256d mining power at the chain, mined 102 blocks, then withdrew — leaving difficulty at 1.7 billion and block times of 52 minutes instead of 60 seconds.

This attack exposed the fundamental weakness of DGW: it adjusts gradually over 24 blocks, leaving small chains vulnerable to sustained periods of abnormal difficulty after a hashrate spike or withdrawal.

QUD is designed to make this class of attack economically irrational and technically ineffective.

---

## Specification

### Activation

QUD activates at block **6,000** on mainnet. Before this height, DGW continues to operate unchanged. At block 6,000, QUD takes over difficulty calculation for all subsequent blocks.

| Network | Activation Height |
|---|---|
| Mainnet | 6,000 |
| Testnet5 | 100 |
| Regtest | 0 |

---

## Pillar 1 — Dual Window Observation

QUD observes the blockchain through two simultaneous windows:

- **Short window:** last 8 blocks — detects anomalies rapidly
- **Long window:** last 144 blocks — provides stability anchor

Under normal conditions, the two windows are blended:
blendedAvg = (0.4 × shortAvg) + (0.6 × longAvg)
This means the long window dominates during stable operation, preventing overreaction to short-term noise. Organic hashrate growth from price appreciation is handled smoothly — difficulty rises proportionally with the long window as miners join gradually over days and weeks.

---

## Pillar 2 — Emergency Mode

If the short window average block time deviates more than **300%** from the 60-second target in either direction, Emergency Mode activates automatically.

In Emergency Mode, the blend inverts:
blendedAvg = (0.9 × shortAvg) + (0.1 × longAvg)
The chain now reacts almost entirely to recent block times, responding to a hashrate attack or crash within 3–5 blocks instead of 24.

Emergency Mode deactivates once block times return within **150%** of the 60-second target for **3 consecutive blocks**.

---

## Pillar 3 — Heisenberg Uncertainty Cap

The maximum difficulty adjustment per block is not a fixed value. It is derived deterministically from the lowest 8 bits of the previous block's hash:
uncertainty_bits = prevBlockHash & 0xFF
uncertainty_cap  = 0.12 + (uncertainty_bits / 255.0) × 0.08
This produces a cap between **12% and 20%** per block adjustment, inclusive.

The cap is verifiable by every full node but **unknowable to any miner before they solve the block** — because the block hash cannot be predicted without performing the proof of work.

**The consequence:** An attacker cannot pre-calculate exactly how many blocks they need to mine to push difficulty to a specific target. The uncertainty is baked into the protocol at the quantum level — named after Heisenberg's Uncertainty Principle, which states that certain properties of a system cannot be known simultaneously with precision.

In Emergency Mode, the uncertainty cap **doubles** — allowing faster correction when the chain is under attack.

---

## Pillar 4 — Dead Zone Recovery

If no block is found for **10 minutes** (10× the target block time), the chain enters the Dead Zone:

- **At 10 minutes:** difficulty drops 50% immediately
- **At 20 minutes:** difficulty drops another 50%
- **At 30 minutes:** difficulty drops another 50%

Each drop is applied as soon as the next block arrives and the elapsed time is verified. The Dead Zone exits the moment a valid block is found and normal QUD calculation resumes from that point.

---

## Pillar 5 — Timestamp Vise

Miners can lie about block timestamps to game difficulty algorithms. QUD neutralises this attack:

1. Calculate the **rolling median** of the last 11 block timestamps
2. Calculate the **standard deviation** of those 11 timestamps
3. If any block's timestamp deviates more than **2 standard deviations** from the median, clamp it to the median before feeding it into the difficulty calculation

A miner who submits a manipulated timestamp gains no advantage — the Timestamp Vise silently corrects the input before the difficulty algorithm sees it.

---

## Pillar 6 — Attack Memory

When Emergency Mode activates, QUD records a **spike signature** into a 1,000-block rolling memory:
spike_signature = {
magnitude:  shortAvg / targetTime,
duration:   number of blocks in emergency mode,
direction:  UP (hashrate spike) or DOWN (hashrate withdrawal)
}
If a statistically similar spike pattern appears within the 1,000-block window, Emergency Mode activates in **2 blocks** instead of waiting for the normal 300% threshold.

The chain remembers being attacked and responds faster the second time.

---

## Pillar 7 — Asymmetric Response

Difficulty adjusts **asymmetrically**:

- **Rising difficulty:** full adjustment speed (100%)
- **Falling difficulty:** 60% of calculated adjustment speed
if rawAdj > 1.0:
finalAdj = min(rawAdj, 1.0 + uncertainty_cap)
else:
slowedAdj = 1.0 - ((1.0 - rawAdj) × 0.6)
finalAdj = max(slowedAdj, 1.0 - uncertainty_cap)
This makes attack-withdraw cycles economically painful — the attacker pays the high difficulty cost but the chain does not crash immediately after they leave.

---

## Complete QUD Algorithm Per Block
INPUT:  pindexPrev (previous block index)
OUTPUT: nBits (next block difficulty target)

TIMESTAMP VISE

Collect timestamps of last 11 blocks
Calculate median and standard deviation
Clamp any outlier timestamps to median


COLLECT WINDOWS

shortTimes[] = clamped timestamps of last 8 blocks
longTimes[]  = clamped timestamps of last 144 blocks
shortAvg = average(shortTimes)
longAvg  = average(longTimes)


DEAD ZONE CHECK

elapsed = now - pindexPrev.nTime
if elapsed > 600s:  apply 50% difficulty drop, return
if elapsed > 1200s: apply additional 50% drop, return
if elapsed > 1800s: apply additional 50% drop, return


DETERMINE MODE

deviation = abs(shortAvg - TARGET_TIME) / TARGET_TIME
if deviation > 3.0: EMERGENCY MODE
else: check Attack Memory for matching signature

if match: EMERGENCY MODE (fast activation in 2 blocks)
else: NORMAL MODE




BLEND

NORMAL:    blendedAvg = 0.4 × shortAvg + 0.6 × longAvg
EMERGENCY: blendedAvg = 0.9 × shortAvg + 0.1 × longAvg


RAW ADJUSTMENT

rawAdj = TARGET_TIME / blendedAvg


HEISENBERG CAP

uncertainty_bits = prevBlockHash & 0xFF
cap = 0.12 + (uncertainty_bits / 255.0) × 0.08
if EMERGENCY MODE: cap × 2
Apply asymmetric response and cap


ATTACK MEMORY UPDATE

if EMERGENCY MODE was active: record spike signature


RETURN

newTarget = currentTarget × finalAdj
nBits = CompressTarget(newTarget)
---

## Void Collapse Bounty Block

### Concept

Named after quantum wave function collapse — the moment a miner solves a block, the superposition collapses and they discover whether it is a Void Collapse Bounty Block. Until that moment, every block exists simultaneously as a bounty block and not a bounty block — **Schrödinger's Block**.

### Trigger Mechanism

After a block is solved, the first 3 bytes of the resulting block hash are compared against a threshold:
bounty_value = (blockHash[0] << 16) | (blockHash[1] << 8) | blockHash[2]
is_bounty    = bounty_value < BOUNTY_THRESHOLD
Where `BOUNTY_THRESHOLD = 33,554` produces a probability of approximately **1 in 500 blocks**.

### Bounty Reward

When triggered:
coinbase reward = 200 VOID (instead of normal block reward)
### Bounty Shutdown Zone

The Void Collapse Bounty Block is permanently disabled when remaining unmined supply drops to **200 VOID or less** before tail emission activates:
if (MAX_MONEY - totalMinedSupply) <= 200 * COIN:
bounty_blocks_enabled = false
---

## Tail Emission Floor

After the final halving, a permanent floor of **0.1 VOID per block** ensures miners are always incentivised:
if nSubsidy == 0:
nSubsidy = 0.1 * COIN
Annual emission at tail: ~52,560 VOID (<0.023% inflation at max supply).
Void Collapse Bounty Blocks are permanently disabled during tail emission.

---

## Emission Lifecycle Summary

| Phase | Block Range | Normal Reward | Bounty Block | Notes |
|---|---|---|---|---|
| Pre-QUD | 1 → 5,999 | 50 VOID (halving) | ❌ | DGW active |
| QUD Active | 6,000 → final halving | 50 VOID (halving) | ✅ 200 VOID | QUD + bounty live |
| Shutdown Zone | Last 200 VOID before cap | Normal reward | ❌ Disabled | Supply protection |
| Tail Emission | Post final halving | 0.1 VOID forever | ❌ Disabled | Chain security floor |

---

## Comparison With Existing Algorithms

| Feature | Bitcoin | DGW | LWMA | ASERT | QUD |
|---|---|---|---|---|---|
| Dual window blend | ❌ | ❌ | ❌ | ❌ | ✅ |
| Emergency mode | ❌ | ❌ | ❌ | ❌ | ✅ |
| Hash-derived uncertainty cap | ❌ | ❌ | ❌ | ❌ | ✅ |
| Dead zone recovery | ❌ | ❌ | ❌ | ❌ | ✅ |
| Timestamp vise | ❌ | Partial | Partial | ❌ | ✅ |
| Attack memory | ❌ | ❌ | ❌ | ❌ | ✅ |
| Asymmetric response | ❌ | ❌ | Partial | ❌ | ✅ |
| Consensus bounty block | ❌ | ❌ | ❌ | ❌ | ✅ |
| Tail emission | ❌ | ❌ | ❌ | ❌ | ✅ |

---

## Prior Art Statement

The following are confirmed original to VoidCoin as of 2026-07-04:

- Dual-window difficulty blend with dynamic emergency mode switching
- Block-hash-derived Heisenberg Uncertainty Cap
- Attack Memory spike fingerprinting
- Void Collapse Bounty Block (hash-triggered random consensus reward)
- Bounty Block Shutdown Zone (supply-aware bounty disabling)
- Complete QUD algorithm combining all seven pillars

---

## Reference Implementation

Branch: `feature/qud-difficulty`
Repository: `https://github.com/VoidHash-Crypto/VoidCoin-Core`
Target release: v1.0.4
Activation: Block 6,000 mainnet

Files to be modified:
- `src/pow.cpp` — QUD algorithm
- `src/pow.h` — QUD declarations
- `src/validation.cpp` — GetBlockSubsidy (bounty + tail emission)
- `src/consensus/params.h` — activation heights
- `src/kernel/chainparams.cpp` — activation height assignments + checkpoint

---

## Copyright

Copyright (c) 2026 VoidCoin Development Team. All rights reserved.

VoidCoin™, Quantum Uncertainty Difficulty™, QUD™, Void Collapse™, Heisenberg Cap™, and Attack Memory™ are trademarks of VoidCoin.

This document is timestamped on GitHub on 2026-07-04 as the original work of the VoidCoin Development Team.
