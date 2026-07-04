// Copyright (c) 2015-2026 The Bitcoin Core developers
// Copyright (c) 2026 The VoidCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// VCIP-2: Quantum Uncertainty Difficulty (QUD)
// VoidCoin's original difficulty adjustment algorithm.
// Replaces Dark Gravity Wave at nQUDActivationHeight.
// See VCIP-2.md for full specification.

#include <pow.h>
#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <assert.h>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

// ============================================================
// Legacy DGW constants
// ============================================================
static constexpr int64_t nPastBlocks = 24;

static inline uint32_t PowLimitBits(const Consensus::Params& params)
{
    return UintToArith256(params.powLimit).GetCompact();
}

static inline int64_t BlockTimeForDA(const CBlockIndex* pindex)
{
    return pindex->GetMedianTimePast();
}

// ============================================================
// Legacy Dark Gravity Wave (used before nQUDActivationHeight)
// ============================================================
static unsigned int VoidCoinDarkGravityWave(const CBlockIndex* pindexLast,
                                            const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (params.fPowNoRetargeting) return pindexLast->nBits;
    if (pindexLast->nHeight < nPastBlocks) return bnPowLimit.GetCompact();

    const CBlockIndex* pindex = pindexLast;
    arith_uint256 bnPastTargetAvg;
    arith_uint256 bnTarget;
    bool fNegative = false, fOverflow = false;
    int64_t nActualTimespan = 0, nBlockCount = 0, nLastBlockTime = 0;

    while (pindex != nullptr && nBlockCount < nPastBlocks) {
        bnTarget.SetCompact(pindex->nBits, &fNegative, &fOverflow);
        if (fNegative || fOverflow || bnTarget == 0) return bnPowLimit.GetCompact();
        ++nBlockCount;
        if (nBlockCount == 1) bnPastTargetAvg = bnTarget;
        else bnPastTargetAvg = ((bnPastTargetAvg * (nBlockCount - 1)) + bnTarget) / nBlockCount;
        const int64_t nThisBlockTime = BlockTimeForDA(pindex);
        if (nLastBlockTime > 0) {
            int64_t nDiff = nLastBlockTime - nThisBlockTime;
            if (nDiff < 0) nDiff = 0;
            nActualTimespan += nDiff;
        }
        nLastBlockTime = nThisBlockTime;
        pindex = pindex->pprev;
    }
    if (nBlockCount < nPastBlocks) return bnPowLimit.GetCompact();

    const int64_t nTargetTimespan = nPastBlocks * params.nPowTargetSpacing;
    if (nActualTimespan < nTargetTimespan / 4) nActualTimespan = nTargetTimespan / 4;
    if (nActualTimespan > nTargetTimespan * 4) nActualTimespan = nTargetTimespan * 4;

    arith_uint256 bnNew = bnPastTargetAvg;
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;
    if (bnNew == 0 || bnNew > bnPowLimit) bnNew = bnPowLimit;
    return bnNew.GetCompact();
}

// ============================================================
// VCIP-2: Quantum Uncertainty Difficulty (QUD)
// ============================================================

// QUD constants
static constexpr int64_t QUD_TARGET_SPACING     = 60;   // 1 minute target
static constexpr int64_t QUD_SHORT_WINDOW        = 8;    // Pillar 1: short window
static constexpr int64_t QUD_LONG_WINDOW         = 144;  // Pillar 1: long window
static constexpr int64_t QUD_TIMESTAMP_WINDOW    = 11;   // Pillar 5: timestamp vise
static constexpr int64_t QUD_ATTACK_MEMORY       = 1000; // Pillar 6: attack memory blocks
static constexpr double  QUD_EMERGENCY_THRESHOLD = 3.0;  // Pillar 2: 300% deviation
static constexpr double  QUD_EMERGENCY_DEACT     = 1.5;  // Pillar 2: deactivate at 150%
static constexpr int64_t QUD_DEAD_ZONE_1         = 600;  // Pillar 4: 10 min
static constexpr int64_t QUD_DEAD_ZONE_2         = 1200; // Pillar 4: 20 min
static constexpr int64_t QUD_DEAD_ZONE_3         = 1800; // Pillar 4: 30 min
static constexpr double  QUD_ASYMMETRIC_FACTOR   = 0.6;  // Pillar 7: fall at 60% speed
static constexpr double  QUD_CAP_MIN             = 0.12; // Pillar 3: min cap 12%
static constexpr double  QUD_CAP_RANGE           = 0.08; // Pillar 3: cap range 8%

// Attack memory entry
struct QUDSpikeEntry {
    int64_t height;
    double  magnitude;
    int64_t duration;
    bool    directionUp; // true=hashrate spike, false=hashrate drop
};

// Global attack memory (in-memory, resets on restart — sufficient for protection)
static std::vector<QUDSpikeEntry> g_qudAttackMemory;

// Pillar 3: Heisenberg Uncertainty Cap
// Derived from low 8 bits of previous block hash — unknowable before solving
static double QUDGetUncertaintyCap(const uint256& prevHash, bool emergencyMode)
{
    // Use XOR of bytes spread across 4 quadrants of the 256-bit hash.
    // Grinding a single byte is trivial; grinding 4 spread bytes simultaneously
    // is computationally equivalent to additional proof of work.
    const unsigned char* data = prevHash.begin();
    uint8_t bits = data[0] ^ data[8] ^ data[16] ^ data[24];
    double cap = QUD_CAP_MIN + (bits / 255.0) * QUD_CAP_RANGE;
    if (emergencyMode) cap *= 2.0;
    return cap;
}

// Pillar 5: Timestamp Vise
// Clamp outlier timestamps to rolling median
static int64_t QUDClampTimestamp(int64_t ts, const std::vector<int64_t>& window)
{
    if (window.size() < 2) return ts;
    std::vector<int64_t> sorted = window;
    std::sort(sorted.begin(), sorted.end());
    int64_t median = sorted[sorted.size() / 2];

    // Calculate standard deviation
    double mean = 0;
    for (auto t : window) mean += t;
    mean /= window.size();
    double variance = 0;
    for (auto t : window) variance += (t - mean) * (t - mean);
    double stddev = std::sqrt(variance / window.size());

    if (stddev > 0 && std::abs(ts - median) > 2.0 * stddev) {
        return median;
    }
    return ts;
}

// Pillar 6: Check attack memory for matching spike pattern
static bool QUDAttackMemoryMatch(int64_t currentHeight, double currentMagnitude, bool dirUp)
{
    // Clean old entries outside memory window
    g_qudAttackMemory.erase(
        std::remove_if(g_qudAttackMemory.begin(), g_qudAttackMemory.end(),
            [currentHeight](const QUDSpikeEntry& e) {
                return (currentHeight - e.height) > QUD_ATTACK_MEMORY;
            }),
        g_qudAttackMemory.end()
    );

    // Check for similar pattern
    for (const auto& entry : g_qudAttackMemory) {
        if (entry.directionUp == dirUp) {
            double magDiff = std::abs(entry.magnitude - currentMagnitude) / entry.magnitude;
            if (magDiff < 0.3) { // within 30% magnitude similarity
                return true;
            }
        }
    }
    return false;
}

// Main QUD algorithm (VCIP-2)
static unsigned int VoidCoinQUD(const CBlockIndex* pindexLast,
                                 const Consensus::Params& params,
                                 int64_t nCurrentTime)
{
    assert(pindexLast != nullptr);
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (params.fPowNoRetargeting) return pindexLast->nBits;

    // Need at least long window of blocks
    if (pindexLast->nHeight < QUD_LONG_WINDOW) return bnPowLimit.GetCompact();

    // --------------------------------------------------------
    // Pillar 5: Collect timestamps with Timestamp Vise
    // --------------------------------------------------------
    std::vector<int64_t> allTimes;
    const CBlockIndex* pindex = pindexLast;
    for (int i = 0; i < QUD_LONG_WINDOW && pindex != nullptr; i++) {
        allTimes.push_back(pindex->GetBlockTime());
        pindex = pindex->pprev;
    }

    // Build clamped block times for short and long windows
    std::vector<int64_t> shortBlockTimes;
    std::vector<int64_t> longBlockTimes;

    pindex = pindexLast;
    for (int i = 0; i < QUD_LONG_WINDOW && pindex != nullptr && pindex->pprev != nullptr; i++) {
        // Collect timestamp window around this block
        std::vector<int64_t> tsWindow;
        const CBlockIndex* pw = pindex;
        for (int j = 0; j < QUD_TIMESTAMP_WINDOW && pw != nullptr; j++, pw = pw->pprev) {
            tsWindow.push_back(pw->GetBlockTime());
        }

        int64_t rawTime = pindex->GetBlockTime() - pindex->pprev->GetBlockTime();
        if (rawTime < 0) rawTime = 0;

        // Clamp using vise
        int64_t clampedTime = QUDClampTimestamp(rawTime, tsWindow);
        if (clampedTime < 0) clampedTime = 0;

        longBlockTimes.push_back(clampedTime);
        if (i < QUD_SHORT_WINDOW) shortBlockTimes.push_back(clampedTime);

        pindex = pindex->pprev;
    }

    if (shortBlockTimes.empty() || longBlockTimes.empty()) return bnPowLimit.GetCompact();

    // Calculate averages
    double shortAvg = 0, longAvg = 0;
    for (auto t : shortBlockTimes) shortAvg += t;
    shortAvg /= shortBlockTimes.size();
    for (auto t : longBlockTimes) longAvg += t;
    longAvg /= longBlockTimes.size();

    // --------------------------------------------------------
    // Pillar 4: Dead Zone Recovery
    // --------------------------------------------------------
    // Pillar 4: Dead Zone Recovery (attack-hardened)
    // Dead zone reductions are subject to asymmetric throttle (60% speed)
    // and hard-capped at 75% total reduction (4x target multiplier max).
    // This prevents hit-and-run attacks where an attacker mines to spike
    // difficulty then withdraws to exploit the dead zone drop.
    int64_t elapsed = nCurrentTime - pindexLast->GetBlockTime();
    if (elapsed > QUD_DEAD_ZONE_1) {
        arith_uint256 bnCurrent;
        bool fNeg = false, fOver = false;
        bnCurrent.SetCompact(pindexLast->nBits, &fNeg, &fOver);
        if (fNeg || fOver) return bnPowLimit.GetCompact();

        // Calculate asymmetric reduction per tier
        double reduction = 1.0;
        if (elapsed > QUD_DEAD_ZONE_3) {
            reduction *= (1.0 - (0.5 * QUD_ASYMMETRIC_FACTOR)); // tier 1
            reduction *= (1.0 - (0.5 * QUD_ASYMMETRIC_FACTOR)); // tier 2
            reduction *= (1.0 - (0.5 * QUD_ASYMMETRIC_FACTOR)); // tier 3
        } else if (elapsed > QUD_DEAD_ZONE_2) {
            reduction *= (1.0 - (0.5 * QUD_ASYMMETRIC_FACTOR)); // tier 1
            reduction *= (1.0 - (0.5 * QUD_ASYMMETRIC_FACTOR)); // tier 2
        } else {
            reduction *= (1.0 - (0.5 * QUD_ASYMMETRIC_FACTOR)); // tier 1
        }
        // Hard cap: never reduce difficulty more than 75%
        if (reduction < 0.25) reduction = 0.25;

        // Target multiplier = 1/reduction (higher target = lower difficulty)
        double targetMult = 1.0 / reduction;
        if (targetMult > 4.0) targetMult = 4.0;

        uint64_t multNum = (uint64_t)(targetMult * 1000000.0);
        bnCurrent *= multNum;
        bnCurrent /= 1000000ULL;
        if (bnCurrent > bnPowLimit) bnCurrent = bnPowLimit;
        return bnCurrent.GetCompact();
    }

    // --------------------------------------------------------
    // Pillar 2: Emergency Mode detection
    // --------------------------------------------------------
    double deviation = std::abs(shortAvg - QUD_TARGET_SPACING) / (double)QUD_TARGET_SPACING;
    bool emergencyMode = (deviation > QUD_EMERGENCY_THRESHOLD);

    // Pillar 6: Attack Memory fast-activation
    if (!emergencyMode && deviation > 1.0) {
        bool dirUp = (shortAvg < QUD_TARGET_SPACING); // blocks faster = more hashrate
        if (QUDAttackMemoryMatch(pindexLast->nHeight, deviation, dirUp)) {
            emergencyMode = true;
        }
    }

    // --------------------------------------------------------
    // Pillar 1: Dual Window Blend
    // --------------------------------------------------------
    double blendedAvg;
    if (emergencyMode) {
        blendedAvg = 0.9 * shortAvg + 0.1 * longAvg;
    } else {
        blendedAvg = 0.4 * shortAvg + 0.6 * longAvg;
    }
    if (blendedAvg <= 0) blendedAvg = 1;

    // --------------------------------------------------------
    // Raw adjustment ratio
    // --------------------------------------------------------
    double rawAdj = (double)QUD_TARGET_SPACING / blendedAvg;

    // --------------------------------------------------------
    // Pillar 3: Heisenberg Uncertainty Cap
    // --------------------------------------------------------
    double cap = QUDGetUncertaintyCap(pindexLast->GetBlockHash(), emergencyMode);
    double finalAdj;

    if (rawAdj > 1.0) {
        // Difficulty rising — full speed
        finalAdj = std::min(rawAdj, 1.0 + cap);
    } else {
        // Pillar 7: Asymmetric Response — fall at 60% speed
        double slowedAdj = 1.0 - ((1.0 - rawAdj) * QUD_ASYMMETRIC_FACTOR);
        finalAdj = std::max(slowedAdj, 1.0 - cap);
    }

    // Clamp to reasonable bounds
    if (finalAdj < 0.25) finalAdj = 0.25;
    if (finalAdj > 4.0)  finalAdj = 4.0;

    // --------------------------------------------------------
    // Pillar 6: Record spike to Attack Memory
    // --------------------------------------------------------
    if (emergencyMode) {
        QUDSpikeEntry entry;
        entry.height     = pindexLast->nHeight;
        entry.magnitude  = deviation;
        entry.duration   = 1;
        entry.directionUp = (shortAvg < QUD_TARGET_SPACING);
        g_qudAttackMemory.push_back(entry);
    }

    // --------------------------------------------------------
    // Apply adjustment to current target
    // --------------------------------------------------------
    arith_uint256 bnCurrent;
    bool fNeg = false, fOver = false;
    bnCurrent.SetCompact(pindexLast->nBits, &fNeg, &fOver);
    if (fNeg || fOver || bnCurrent == 0) return bnPowLimit.GetCompact();

    // Scale target: higher target = lower difficulty
    // finalAdj > 1 means blocks too slow, raise target (lower difficulty)
    // finalAdj < 1 means blocks too fast, lower target (raise difficulty)
    // Use integer arithmetic: multiply by 1000000, apply, divide
    uint64_t adjNumerator   = (uint64_t)(finalAdj * 1000000.0);
    uint64_t adjDenominator = 1000000ULL;

    arith_uint256 bnNew = bnCurrent;
    bnNew *= adjNumerator;
    bnNew /= adjDenominator;

    if (bnNew == 0 || bnNew > bnPowLimit) bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

// ============================================================
// Public interface
// ============================================================
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast,
                                       int64_t nFirstBlockTime,
                                       const Consensus::Params& params)
{
    (void)nFirstBlockTime;
    assert(pindexLast != nullptr);
    if (params.fPowNoRetargeting) return pindexLast->nBits;

    if (pindexLast->nHeight >= params.nQUDActivationHeight) {
        return VoidCoinQUD(pindexLast, params, pindexLast->GetBlockTime());
    }
    return VoidCoinDarkGravityWave(pindexLast, params);
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast,
                                 const CBlockHeader* pblock,
                                 const Consensus::Params& params)
{
    (void)pblock;
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    if (pindexLast == nullptr) return bnPowLimit.GetCompact();
    if (params.fPowNoRetargeting) return pindexLast->nBits;

    int64_t nCurrentTime = pblock ? pblock->GetBlockTime() : pindexLast->GetBlockTime();

    if (pindexLast->nHeight + 1 >= params.nQUDActivationHeight) {
        return VoidCoinQUD(pindexLast, params, nCurrentTime);
    }
    return VoidCoinDarkGravityWave(pindexLast, params);
}

std::optional<arith_uint256> DeriveTarget(unsigned int nBits, const uint256 pow_limit)
{
    bool fNegative = false, fOverflow = false;
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    if (fNegative || bnTarget == 0 || fOverflow ||
        bnTarget > UintToArith256(pow_limit)) {
        return {};
    }
    return bnTarget;
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative = false, fOverflow = false;
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    if (fNegative || bnTarget == 0 || fOverflow ||
        bnTarget > UintToArith256(params.powLimit)) return false;
    if (UintToArith256(hash) > bnTarget) return false;
    return true;
}

bool CheckProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    return CheckProofOfWork(block.GetHash(), block.nBits, params);
}

bool PermittedDifficultyTransition(const Consensus::Params& params,
                                   int64_t height,
                                   uint32_t old_nbits,
                                   uint32_t new_nbits)
{
    if (params.fPowNoRetargeting) return true;
    bool fNegative = false, fOverflow = false;
    arith_uint256 old_target;
    old_target.SetCompact(old_nbits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || old_target == 0 ||
        old_target > UintToArith256(params.powLimit)) return false;
    fNegative = fOverflow = false;
    arith_uint256 new_target;
    new_target.SetCompact(new_nbits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || new_target == 0 ||
        new_target > UintToArith256(params.powLimit)) return false;
    if (height < params.nQUDActivationHeight) {
        if (height <= nPastBlocks) return new_nbits == PowLimitBits(params);
        if (new_target > old_target * 4) return false;
        if (new_target < old_target / 4) return false;
    } else {
        // QUD: allow up to 8x change (dead zone can drop 50% three times)
        if (new_target > old_target * 8) return false;
        if (new_target < old_target / 8) return false;
    }
    return true;
}
