/*
* Define useful compiler-specific macros
* (C) 2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

/* This header is included in both C++ and C (via ffi.h) and should only
   contain macro definitions. Avoid C++ style // comments in this file.
*/

#ifndef BOTAN_UTIL_COMPILER_FLAGS_H_
#define BOTAN_UTIL_COMPILER_FLAGS_H_

/* Should we use GCC-style inline assembler? */
#if defined(BOTAN_BUILD_COMPILER_IS_GCC) || \
   defined(BOTAN_BUILD_COMPILER_IS_CLANG) || \
   defined(BOTAN_BUILD_COMPILER_IS_XLC) || \
   defined(BOTAN_BUILD_COMPILER_IS_SUN_STUDIO)

  #define BOTAN_USE_GCC_INLINE_ASM
#endif

/**
* Used to annotate API exports which are public and supported.
* These APIs will not be broken/removed unless strictly required for
* functionality or security, and only in new major versions.
* @param maj The major version this public API was released in
* @param min The minor version this public API was released in
*/
#define BOTAN_PUBLIC_API(maj,min) BOTAN_DLL

/**
* Used to annotate API exports which are public, but are now deprecated
* and which will be removed in a future major release.
*/
#define BOTAN_DEPRECATED_API(msg) BOTAN_DLL BOTAN_DEPRECATED(msg)

/**
* Used to annotate API exports which are public and can be used by
* applications if needed, but which are intentionally not documented,
* and which may change incompatibly in a future major version.
*/
#define BOTAN_UNSTABLE_API BOTAN_DLL

/**
* Used to annotate API exports which are exported but only for the
* purposes of testing. They should not be used by applications and
* may be removed or changed without notice.
*/
#define BOTAN_TEST_API BOTAN_DLL

/*
* Define BOTAN_GCC_VERSION
*/
#if defined(__GNUC__) && !defined(__clang__)
  #define BOTAN_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__ * 10 + __GNUC_PATCHLEVEL__)
#else
  #define BOTAN_GCC_VERSION 0
#endif

/*
* Define BOTAN_CLANG_VERSION
*/
#if defined(__clang__)
  #define BOTAN_CLANG_VERSION (__clang_major__ * 10 + __clang_minor__)
#else
  #define BOTAN_CLANG_VERSION 0
#endif

/*
* Define BOTAN_FUNC_ISA
*/
#if (defined(__GNUC__) && !defined(__clang__)) || (BOTAN_CLANG_VERSION > 38)
  #define BOTAN_FUNC_ISA(isa) __attribute__ ((target(isa)))
#else
  #define BOTAN_FUNC_ISA(isa)
#endif

/*
* Define BOTAN_WARN_UNUSED_RESULT
*/
#if defined(__GNUC__) || defined(__clang__)
  #define BOTAN_WARN_UNUSED_RESULT __attribute__ ((warn_unused_result))
#else
  #define BOTAN_WARN_UNUSED_RESULT
#endif

/*
* Define BOTAN_MALLOC_FN
*/
#if defined(__ibmxl__)
  /* XLC pretends to be both Clang and GCC, but is neither */
  #define BOTAN_MALLOC_FN __attribute__ ((malloc))
#elif defined(__GNUC__)
  #define BOTAN_MALLOC_FN __attribute__ ((malloc, alloc_size(1,2)))
#elif defined(_MSC_VER)
  #define BOTAN_MALLOC_FN __declspec(restrict)
#else
  #define BOTAN_MALLOC_FN
#endif

/*
* Define BOTAN_DEPRECATED
*/
#if !defined(BOTAN_NO_DEPRECATED_WARNINGS) && !defined(BOTAN_IS_BEING_BUILT) && !defined(BOTAN_AMALGAMATION_H_)

  #if defined(__clang__)
    #define BOTAN_DEPRECATED(msg) __attribute__ ((deprecated(msg)))
    #define BOTAN_DEPRECATED_HEADER(hdr) _Pragma("message \"this header is deprecated\"")
    #define BOTAN_FUTURE_INTERNAL_HEADER(hdr) _Pragma("message \"this header will be made internal in the future\"")

  #elif defined(_MSC_VER)
    #define BOTAN_DEPRECATED(msg) __declspec(deprecated(msg))
    #define BOTAN_DEPRECATED_HEADER(hdr) __pragma(message("this header is deprecated"))
    #define BOTAN_FUTURE_INTERNAL_HEADER(hdr) __pragma(message("this header will be made internal in the future"))

  #elif defined(__GNUC__)
    /* msg supported since GCC 4.5, earliest we support is 4.8 */
    #define BOTAN_DEPRECATED(msg) __attribute__ ((deprecated(msg)))
    #define BOTAN_DEPRECATED_HEADER(hdr) _Pragma("GCC warning \"this header is deprecated\"")
    #define BOTAN_FUTURE_INTERNAL_HEADER(hdr) _Pragma("GCC warning \"this header will be made internal in the future\"")
  #endif

#endif

#if !defined(BOTAN_DEPRECATED)
  #define BOTAN_DEPRECATED(msg)
#endif

#if !defined(BOTAN_DEPRECATED_HEADER)
  #define BOTAN_DEPRECATED_HEADER(hdr)
#endif

#if !defined(BOTAN_FUTURE_INTERNAL_HEADER)
  #define BOTAN_FUTURE_INTERNAL_HEADER(hdr)
#endif

/*
* Define BOTAN_NORETURN
*/
#if !defined(BOTAN_NORETURN)

  #if defined (__clang__) || defined (__GNUC__)
    #define BOTAN_NORETURN __attribute__ ((__noreturn__))

  #elif defined (_MSC_VER)
    #define BOTAN_NORETURN __declspec(noreturn)

  #else
    #define BOTAN_NORETURN
  #endif

#endif

/*
* Define BOTAN_THREAD_LOCAL
*/
#if !defined(BOTAN_THREAD_LOCAL)

  #if defined(BOTAN_TARGET_OS_HAS_THREADS) && defined(BOTAN_TARGET_OS_HAS_THREAD_LOCAL)
    #define BOTAN_THREAD_LOCAL thread_local
  #else
    #define BOTAN_THREAD_LOCAL /**/
  #endif

#endif

/*
* Define BOTAN_IF_CONSTEXPR
*/
#if !defined(BOTAN_IF_CONSTEXPR)
   #if __cplusplus >= 201703
      #define BOTAN_IF_CONSTEXPR if constexpr
   #else
      #define BOTAN_IF_CONSTEXPR if
   #endif
#endif

/*
* Define BOTAN_PARALLEL_FOR
*/
#if !defined(BOTAN_PARALLEL_FOR)

#if defined(BOTAN_TARGET_HAS_OPENMP)
  #define BOTAN_PARALLEL_FOR _Pragma("omp parallel for") for
#else
  #define BOTAN_PARALLEL_FOR for
#endif

#endif

/*
* Define BOTAN_FORCE_INLINE
*/
#if !defined(BOTAN_FORCE_INLINE)

  #if defined (__clang__) || defined (__GNUC__)
    #define BOTAN_FORCE_INLINE __attribute__ ((__always_inline__)) inline

  #elif defined (_MSC_VER)
    #define BOTAN_FORCE_INLINE __forceinline

  #else
    #define BOTAN_FORCE_INLINE inline
  #endif

#endif

/*
* Define BOTAN_PARALLEL_SIMD_FOR
*/
#if !defined(BOTAN_PARALLEL_SIMD_FOR)

#if defined(BOTAN_TARGET_HAS_OPENMP)
  #define BOTAN_PARALLEL_SIMD_FOR _Pragma("omp simd") for
#elif defined(BOTAN_BUILD_COMPILER_IS_GCC) && (BOTAN_GCC_VERSION >= 490)
  #define BOTAN_PARALLEL_SIMD_FOR _Pragma("GCC ivdep") for
#else
  #define BOTAN_PARALLEL_SIMD_FOR for
#endif

#endif

#endif
