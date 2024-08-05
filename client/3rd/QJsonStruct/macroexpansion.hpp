#pragma once

#define CONCATENATE1(arg1, arg2) CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2) arg1##arg2
#define CONCATENATE(x, y) x##y

#define EXPAND(...) __VA_ARGS__
#define FOR_EACH_1(what, x, ...) what(x)
#define FOR_EACH_2(what, x, ...) what(x) EXPAND(FOR_EACH_1(what, __VA_ARGS__))
#define FOR_EACH_3(what, x, ...) what(x) EXPAND(FOR_EACH_2(what, __VA_ARGS__))
#define FOR_EACH_4(what, x, ...) what(x) EXPAND(FOR_EACH_3(what, __VA_ARGS__))
#define FOR_EACH_5(what, x, ...) what(x) EXPAND(FOR_EACH_4(what, __VA_ARGS__))
#define FOR_EACH_6(what, x, ...) what(x) EXPAND(FOR_EACH_5(what, __VA_ARGS__))
#define FOR_EACH_7(what, x, ...) what(x) EXPAND(FOR_EACH_6(what, __VA_ARGS__))
#define FOR_EACH_8(what, x, ...) what(x) EXPAND(FOR_EACH_7(what, __VA_ARGS__))
#define FOR_EACH_9(what, x, ...) what(x) EXPAND(FOR_EACH_8(what, __VA_ARGS__))
#define FOR_EACH_10(what, x, ...) what(x) EXPAND(FOR_EACH_9(what, __VA_ARGS__))
#define FOR_EACH_11(what, x, ...) what(x) EXPAND(FOR_EACH_10(what, __VA_ARGS__))
#define FOR_EACH_12(what, x, ...) what(x) EXPAND(FOR_EACH_11(what, __VA_ARGS__))
#define FOR_EACH_13(what, x, ...) what(x) EXPAND(FOR_EACH_12(what, __VA_ARGS__))
#define FOR_EACH_14(what, x, ...) what(x) EXPAND(FOR_EACH_13(what, __VA_ARGS__))
#define FOR_EACH_15(what, x, ...) what(x) EXPAND(FOR_EACH_14(what, __VA_ARGS__))
#define FOR_EACH_16(what, x, ...) what(x) EXPAND(FOR_EACH_15(what, __VA_ARGS__))

#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...) EXPAND(FOR_EACH_ARG_N(__VA_ARGS__))
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define FOR_EACH_RSEQ_N() 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define FOR_EACH_(N, what, ...) EXPAND(CONCATENATE(FOR_EACH_, N)(what, __VA_ARGS__))
#define FOR_EACH(what, ...) FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), what, __VA_ARGS__)
#define FOREACH_CALL_FUNC(func, ...) FOR_EACH(func, __VA_ARGS__)

// Bad hack ==========================================================================================================================
#define _2X_FOR_EACH_1(what, x, ...) what(x)
#define _2X_FOR_EACH_2(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_1(what, __VA_ARGS__))
#define _2X_FOR_EACH_3(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_2(what, __VA_ARGS__))
#define _2X_FOR_EACH_4(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_3(what, __VA_ARGS__))
#define _2X_FOR_EACH_5(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_4(what, __VA_ARGS__))
#define _2X_FOR_EACH_6(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_5(what, __VA_ARGS__))
#define _2X_FOR_EACH_7(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_6(what, __VA_ARGS__))
#define _2X_FOR_EACH_8(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_7(what, __VA_ARGS__))
#define _2X_FOR_EACH_9(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_8(what, __VA_ARGS__))
#define _2X_FOR_EACH_10(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_9(what, __VA_ARGS__))
#define _2X_FOR_EACH_11(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_10(what, __VA_ARGS__))
#define _2X_FOR_EACH_12(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_11(what, __VA_ARGS__))
#define _2X_FOR_EACH_13(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_12(what, __VA_ARGS__))
#define _2X_FOR_EACH_14(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_13(what, __VA_ARGS__))
#define _2X_FOR_EACH_15(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_14(what, __VA_ARGS__))
#define _2X_FOR_EACH_16(what, x, ...) what(x) EXPAND(_2X_FOR_EACH_15(what, __VA_ARGS__))
#define _2X_FOR_EACH_(N, what, ...) EXPAND(CONCATENATE(_2X_FOR_EACH_, N)(what, __VA_ARGS__))
#define _2X_FOR_EACH(what, ...) _2X_FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), what, __VA_ARGS__)
#define FOREACH_CALL_FUNC_2(func, ...) _2X_FOR_EACH(func, __VA_ARGS__)

// Bad hack ==========================================================================================================================
#define _3X_FOR_EACH_1(what, x, ...) what(x)
#define _3X_FOR_EACH_2(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_1(what, __VA_ARGS__))
#define _3X_FOR_EACH_3(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_2(what, __VA_ARGS__))
#define _3X_FOR_EACH_4(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_3(what, __VA_ARGS__))
#define _3X_FOR_EACH_5(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_4(what, __VA_ARGS__))
#define _3X_FOR_EACH_6(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_5(what, __VA_ARGS__))
#define _3X_FOR_EACH_7(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_6(what, __VA_ARGS__))
#define _3X_FOR_EACH_8(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_7(what, __VA_ARGS__))
#define _3X_FOR_EACH_9(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_8(what, __VA_ARGS__))
#define _3X_FOR_EACH_10(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_9(what, __VA_ARGS__))
#define _3X_FOR_EACH_11(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_10(what, __VA_ARGS__))
#define _3X_FOR_EACH_12(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_11(what, __VA_ARGS__))
#define _3X_FOR_EACH_13(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_12(what, __VA_ARGS__))
#define _3X_FOR_EACH_14(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_13(what, __VA_ARGS__))
#define _3X_FOR_EACH_15(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_14(what, __VA_ARGS__))
#define _3X_FOR_EACH_16(what, x, ...) what(x) EXPAND(_3X_FOR_EACH_15(what, __VA_ARGS__))
#define _3X_FOR_EACH_(N, what, ...) EXPAND(CONCATENATE(_3X_FOR_EACH_, N)(what, __VA_ARGS__))
#define _3X_FOR_EACH(what, ...) _3X_FOR_EACH_(FOR_EACH_NARG(__VA_ARGS__), what, __VA_ARGS__)
#define FOREACH_CALL_FUNC_3(func, ...) _3X_FOR_EACH(func, __VA_ARGS__)
