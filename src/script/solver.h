// Copyright (c) 2015-2026 The Bitcoin Core developers
// Copyright (c) 2026 The VoidCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// The Solver functions are used by policy and the wallet, but not consensus.

#ifndef VOIDCOIN_SCRIPT_SOLVER_H
#define VOIDCOIN_SCRIPT_SOLVER_H

#include <attributes.h>
#include <script/script.h>
#include <span.h>

#include <string>
#include <optional>
#include <utility>
#include <vector>

class CPubKey;

enum class TxoutType {
    NONSTANDARD,
    // 'standard' transaction types:
    ANCHOR, //!< anyone can spend script
    PUBKEY,
    PUBKEYHASH,
    SCRIPTHASH,
    MULTISIG,
    NULL_DATA, //!< unspendable OP_RETURN script that carries data
    WITNESS_V0_SCRIPTHASH,
    WITNESS_V0_KEYHASH,
    WITNESS_V1_TAPROOT,
    WITNESS_UNKNOWN, //!< Only for Witness versions not already defined above
    //! Native VoidCoin quantum-resistant output.
    //! Not witness. Not OP_RETURN.
    VOIDCOIN_P2QR,
    //! Native VoidCoin quantum-resistant time-locked output (VCIP-1).
    //! scriptPubKey: OP_QuantumQuark <32-byte-program> <4-byte-lockheight-LE>
    //! Mainnet address HRP: vqt  Testnet: tnqt
    VOIDCOIN_P2QR_TIMELOCK,
};

/** Get the name of a TxoutType as a string */
std::string GetTxnOutputType(TxoutType t);

constexpr bool IsPushdataOp(opcodetype opcode)
{
    return opcode > OP_FALSE && opcode <= OP_PUSHDATA4;
}

static constexpr unsigned int VOIDCOIN_P2QR_PROGRAM_SIZE = 32;
//! Extra 4 bytes appended to P2QR script for the lockheight (little-endian uint32)
static constexpr unsigned int VOIDCOIN_P2QR_TIMELOCK_SIZE = 4;
//! Total scriptPubKey size for P2QR-TIMELOCK: opcode(1) + len(1) + program(32) + lockheight(4) = 38
static constexpr unsigned int VOIDCOIN_P2QR_TIMELOCK_SCRIPT_SIZE =
    2 + VOIDCOIN_P2QR_PROGRAM_SIZE + VOIDCOIN_P2QR_TIMELOCK_SIZE;

// Native VOID quantum-resistant output marker.
// Old nodes execute OP_RESERVED and fail closed.
// Upgraded nodes recognize this as OP_QuantumQuark after activation.
static constexpr opcodetype OP_QuantumQuark = OP_RESERVED;
static constexpr opcodetype OP_VOIDCOIN_P2QR = OP_QuantumQuark;

/**
 * Parse a scriptPubKey and identify script type for standard scripts. If
 * successful, returns script type and parsed pubkeys or hashes, depending on
 * the type. For example, for a P2SH script, vSolutionsRet will contain the
 * script hash, for P2PKH it will contain the key hash, etc.
 *
 * @param[in]   scriptPubKey   Script to parse
 * @param[out]  vSolutionsRet  Vector of parsed pubkeys and hashes
 * @return                     The script type. TxoutType::NONSTANDARD represents a failed solve.
 */
TxoutType Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet);

/** Generate a P2PK script for the given pubkey. */
CScript GetScriptForRawPubKey(const CPubKey& pubkey);

/** Determine if script is a "multi_a" script. Returns (threshold, keyspans) if so, and nullopt otherwise.
 *  The keyspans refer to bytes in the passed script. */
std::optional<std::pair<int, std::vector<Span<const unsigned char>>>> MatchMultiA(const CScript& script LIFETIMEBOUND);

/** Generate a multisig script. */
CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys);

#endif // VOIDCOIN_SCRIPT_SOLVER_H
