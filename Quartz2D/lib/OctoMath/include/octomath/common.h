#pragma once
#include <math.h>
#include <stdint.h>

// signed integers
#define i8  int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

// NOTE: I believe these are not platform independent, TODO: FIX THIS!
// signed maxes
#define i8_invalid  INT8_MAX
#define i16_invalid INT16_MAX
#define i32_invalid INT32_MAX
#define i64_invalid INT64_MAX

// unsigned integers
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

// unsigned maxes
#define u8_invalid  0xff
#define u16_invalid 0xffff
#define u32_invalid 0xffff'ffff
#define u64_invalid 0xffff'ffff'ffff'ffff

// floats
#define f32 float

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062
#define TAO (3.1415926535897932384626433832795028841971693993751058209749445923078164062 * 2)
#define HALF_PI (3.1415926535897932384626433832795028841971693993751058209749445923078164062 * 0.5)
#define RADIAN_MULTIPLIER 0.0174532925199432957692369076848861271344287188854172545609719144017100911

#ifndef __cplusplus
	#define true 1
	#define false 0
#endif

#define OCTOMATH