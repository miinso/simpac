#pragma once
/*
 * cpuinfo_supported_isas.hpp
 *
 * Header-only helper that uses the cpuinfo library (https://github.com/pytorch/cpuinfo)
 * to collect the list of supported instruction set extensions on the current machine
 * and return them as std::vector<std::string>.
 *
 * Usage:
 *   #include "cpuinfo_supported_isas.hpp"
 *   auto isas = cpuinfo_utils::supported_isas();
 *   for (auto& f : isas) {
 *       std::cout << f << "\n";
 *   }
 *
 * Notes:
 * - You must link against libcpuinfo (and ensure cpuinfo is initialized).
 * - This helper calls cpuinfo_initialize() internally (idempotent + thread-safe per cpuinfo design).
 * - Only the instruction-set style “has” predicates are gathered (not topology/cache info).
 * - The exact set of feature checks depends on the architecture you compile for.
 *
 * Extend: If you upgrade cpuinfo and new cpuinfo_has_* predicates appear, just add them
 * to the relevant architecture block below.
 */

#include <cpuinfo.h>
#include <vector>
#include <unordered_set>
#include <string>
#include <utility>

namespace cpuinfo_utils {

/*
 * Options to control which groups of ISA features to include.
 * (You can extend this later if you need finer granularity.)
 */
struct IsaFilter {
    bool base       = false;  // Base/legacy capabilities (e.g. CMPXCHG8B, CMOV)
    bool scalar     = false;  // Scalar arithmetic / system (e.g. POPCNT, LZCNT, atomic, etc.)
    bool simd       = true;  // SIMD packs (SSE*/AVX*/NEON/SVE/AMX)
    bool crypto     = false;  // Cryptography (AES/SHA/PMULL/CRC/ARM crypto)
    bool matrix     = false;  // Specialized matrix (AMX, SME, i8mm, dot product)
};

/*
 * Main helper: returns a vector of supported ISA feature names.
 * Parameter:
 *   filter - choose which categories to include.
 */
inline std::vector<std::string> supported_isas(const IsaFilter& filter = {}) {
    std::vector<std::string> out;
    if (!cpuinfo_initialize()) {
        return out; // initialization failed
    }

#if defined(CPUINFO_ARCH_X86) || defined(CPUINFO_ARCH_X86_64)
    // Base / legacy
    if (filter.base) {
        if (cpuinfo_has_x86_cmov()) out.emplace_back("CMOV");
        if (cpuinfo_has_x86_cmpxchg8b()) out.emplace_back("CMPXCHG8B");
        if (cpuinfo_has_x86_cmpxchg16b()) out.emplace_back("CMPXCHG16B");
        if (cpuinfo_has_x86_mmx()) out.emplace_back("MMX");
    }
    // Scalar/system
    if (filter.scalar) {
        if (cpuinfo_has_x86_popcnt()) out.emplace_back("POPCNT");
        if (cpuinfo_has_x86_lzcnt()) out.emplace_back("LZCNT");
        if (cpuinfo_has_x86_rdtsc()) out.emplace_back("RDTSC");
        if (cpuinfo_has_x86_rdtscp()) out.emplace_back("RDTSCP");
        if (cpuinfo_has_x86_fxsave()) out.emplace_back("FXSAVE");
        if (cpuinfo_has_x86_xsave()) out.emplace_back("XSAVE");
        if (cpuinfo_has_x86_misaligned_sse()) out.emplace_back("MISALIGNED_SSE");
        if (cpuinfo_has_x86_hle()) out.emplace_back("HLE");
        if (cpuinfo_has_x86_rtm()) out.emplace_back("RTM");
        if (cpuinfo_has_x86_mpx()) out.emplace_back("MPX");
        if (cpuinfo_has_x86_xtest()) out.emplace_back("XTEST");
    }
    // SIMD
    if (filter.simd) {
        if (cpuinfo_has_x86_sse()) out.emplace_back("SSE");
        if (cpuinfo_has_x86_sse2()) out.emplace_back("SSE2");
        if (cpuinfo_has_x86_sse3()) out.emplace_back("SSE3");
        if (cpuinfo_has_x86_ssse3()) out.emplace_back("SSSE3");
        if (cpuinfo_has_x86_sse4_1()) out.emplace_back("SSE4.1");
        if (cpuinfo_has_x86_sse4_2()) out.emplace_back("SSE4.2");
        if (cpuinfo_has_x86_sse4a()) out.emplace_back("SSE4A");
        if (cpuinfo_has_x86_avx()) out.emplace_back("AVX");
        if (cpuinfo_has_x86_f16c()) out.emplace_back("F16C");
        if (cpuinfo_has_x86_fma3()) out.emplace_back("FMA3");
        if (cpuinfo_has_x86_fma4()) out.emplace_back("FMA4");
        if (cpuinfo_has_x86_xop()) out.emplace_back("XOP");
        if (cpuinfo_has_x86_avx2()) out.emplace_back("AVX2");

        // AVX-512 families
        if (cpuinfo_has_x86_avx512f()) out.emplace_back("AVX512F");
        if (cpuinfo_has_x86_avx512cd()) out.emplace_back("AVX512CD");
        if (cpuinfo_has_x86_avx512er()) out.emplace_back("AVX512ER");
        if (cpuinfo_has_x86_avx512pf()) out.emplace_back("AVX512PF");
        if (cpuinfo_has_x86_avx512dq()) out.emplace_back("AVX512DQ");
        if (cpuinfo_has_x86_avx512bw()) out.emplace_back("AVX512BW");
        if (cpuinfo_has_x86_avx512vl()) out.emplace_back("AVX512VL");
        if (cpuinfo_has_x86_avx512ifma()) out.emplace_back("AVX512IFMA");
        if (cpuinfo_has_x86_avx512vbmi()) out.emplace_back("AVX512VBMI");
        if (cpuinfo_has_x86_avx512vbmi2()) out.emplace_back("AVX512VBMI2");
        if (cpuinfo_has_x86_avx512bitalg()) out.emplace_back("AVX512BITALG");
        if (cpuinfo_has_x86_avx512vpopcntdq()) out.emplace_back("AVX512VPOPCNTDQ");
        if (cpuinfo_has_x86_avx512vnni()) out.emplace_back("AVX512VNNI");
        if (cpuinfo_has_x86_avx512bf16()) out.emplace_back("AVX512BF16");
        if (cpuinfo_has_x86_avx512fp16()) out.emplace_back("AVX512FP16");
        if (cpuinfo_has_x86_avx512vp2intersect()) out.emplace_back("AVX512VP2INTERSECT");
        if (cpuinfo_has_x86_avx512_4vnniw()) out.emplace_back("AVX512_4VNNIW");
        if (cpuinfo_has_x86_avx512_4fmaps()) out.emplace_back("AVX512_4FMAPS");

        // Emerging / combined
        if (cpuinfo_has_x86_avx10_1()) out.emplace_back("AVX10.1");
        if (cpuinfo_has_x86_avx10_2()) out.emplace_back("AVX10.2");

        // AMX
        if (filter.matrix) {
            if (cpuinfo_has_x86_amx_tile()) out.emplace_back("AMX_TILE");
            if (cpuinfo_has_x86_amx_int8()) out.emplace_back("AMX_INT8");
            if (cpuinfo_has_x86_amx_bf16()) out.emplace_back("AMX_BF16");
            if (cpuinfo_has_x86_amx_fp16()) out.emplace_back("AMX_FP16");
        }

        // AVX VNNI & conversions
        if (cpuinfo_has_x86_avxvnni()) out.emplace_back("AVXVNNI");
        if (cpuinfo_has_x86_avx_vnni_int8()) out.emplace_back("AVX_VNNI_INT8");
        if (cpuinfo_has_x86_avx_vnni_int16()) out.emplace_back("AVX_VNNI_INT16");
        if (cpuinfo_has_x86_avx_ne_convert()) out.emplace_back("AVX_NE_CONVERT");
    }
    // Crypto
    if (filter.crypto) {
        if (cpuinfo_has_x86_aes()) out.emplace_back("AES");
        if (cpuinfo_has_x86_sha()) out.emplace_back("SHA");
    }

#elif defined(CPUINFO_ARCH_ARM) || defined(CPUINFO_ARCH_ARM64)
    // Base / mandatory-ish
    if (filter.base) {
        // (ARM base flags are less granular in cpuinfo public API; focus on extensions)
        // No direct base CPU flag insertion.
    }
    // SIMD (NEON + SVE/SME)
    if (filter.simd) {
        if (cpuinfo_has_arm_neon()) out.emplace_back("NEON");
        if (cpuinfo_has_arm_neon_fp16()) out.emplace_back("NEON_FP16");
        if (cpuinfo_has_arm_neon_fma()) out.emplace_back("NEON_FMA");
        if (cpuinfo_has_arm_neon_v8()) out.emplace_back("NEON_V8");
        if (cpuinfo_has_arm_neon_rdm()) out.emplace_back("NEON_RDM");
        if (cpuinfo_has_arm_neon_fp16_arith()) out.emplace_back("NEON_FP16_ARITH");
        if (cpuinfo_has_arm_neon_dot()) out.emplace_back("NEON_DOT");
        if (cpuinfo_has_arm_neon_bf16()) out.emplace_back("NEON_BF16");
        // SVE / SVE2 / SME (if cpuinfo built with those checks)
        if (cpuinfo_has_arm_sve()) out.emplace_back("SVE");
        if (cpuinfo_has_arm_sve2()) out.emplace_back("SVE2");
        if (cpuinfo_has_arm_sme()) out.emplace_back("SME");
        if (cpuinfo_has_arm_sme2()) out.emplace_back("SME2");
    }
    // Scalar / atomics / FP16 / BF16 / int8 / FHM
    if (filter.scalar) {
        if (cpuinfo_has_arm_fp16_arith()) out.emplace_back("FP16_ARITH");
        if (cpuinfo_has_arm_bf16()) out.emplace_back("BF16");
        if (cpuinfo_has_arm_fhm()) out.emplace_back("FHM");
        if (cpuinfo_has_arm_atomics()) out.emplace_back("ATOMICS");
        if (cpuinfo_has_arm_i8mm()) out.emplace_back("I8MM");
        if (cpuinfo_has_arm_jscvt()) out.emplace_back("JSCVT");
        if (cpuinfo_has_arm_fcma()) out.emplace_back("FCMA");
    }
    // Crypto
    if (filter.crypto) {
        if (cpuinfo_has_arm_aes()) out.emplace_back("AES");
        if (cpuinfo_has_arm_sha1()) out.emplace_back("SHA1");
        if (cpuinfo_has_arm_sha2()) out.emplace_back("SHA2");
        if (cpuinfo_has_arm_pmull()) out.emplace_back("PMULL");
        if (cpuinfo_has_arm_crc32()) out.emplace_back("CRC32");
    }
    // Matrix-like (some already added)
    if (filter.matrix) {
        // Already included: I8MM, NEON_DOT, BF16 if present (they’re matrix / dot relevant)
        // Additional specialized flags could be added here if cpuinfo exposes more.
    }

#elif defined(CPUINFO_ARCH_RISCV32) || defined(CPUINFO_ARCH_RISCV64)
    // RISC-V: gather standard extension flags (cpuinfo may expose these as has_riscv_*).
    if (filter.base) {
        if (cpuinfo_has_riscv_i()) out.emplace_back("RV32I/RV64I");
    }
    if (filter.scalar) {
        if (cpuinfo_has_riscv_m()) out.emplace_back("M");
        if (cpuinfo_has_riscv_a()) out.emplace_back("A");
        if (cpuinfo_has_riscv_f()) out.emplace_back("F");
        if (cpuinfo_has_riscv_d()) out.emplace_back("D");
        if (cpuinfo_has_riscv_c()) out.emplace_back("C");
        if (cpuinfo_has_riscv_v()) out.emplace_back("V");
        if (cpuinfo_has_riscv_zfh()) out.emplace_back("Zfh");
        if (cpuinfo_has_riscv_zba()) out.emplace_back("Zba");
        if (cpuinfo_has_riscv_zbb()) out.emplace_back("Zbb");
        if (cpuinfo_has_riscv_zbc()) out.emplace_back("Zbc");
        if (cpuinfo_has_riscv_zbs()) out.emplace_back("Zbs");
    }
    if (filter.crypto) {
        if (cpuinfo_has_riscv_zknd()) out.emplace_back("Zknd");
        if (cpuinfo_has_riscv_zkne()) out.emplace_back("Zkne");
        if (cpuinfo_has_riscv_zknh()) out.emplace_back("Zknh");
        if (cpuinfo_has_riscv_zkr())  out.emplace_back("Zkr");
        if (cpuinfo_has_riscv_zksed()) out.emplace_back("Zksed");
        if (cpuinfo_has_riscv_zksh()) out.emplace_back("Zksh");
    }
    if (filter.simd) {
        // 'V' is already added (vector extension). Additional sub-extensions could go here.
    }
    if (filter.matrix) {
        // Future matrix/tensor extensions (e.g., RVV advanced) would be added here.
    }

#else
    // Unsupported / unknown architecture in this build; no features.
#endif

    return out;
}

} // namespace cpuinfo_utils