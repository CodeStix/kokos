#pragma once

// This file contains declarations that are used all over kokos

// This attribute is used when a function/struct should not be used anymore and could be removed in a future version, when used anyway, the compiler generates a warning
#define ATTRIBUTE_DEPRECATED __attribute__((deprecated))

// This struct attribute tells the compiler that the location of struct variables may not be changed (this is sometimes done to improve performance by aligning it to a certain address).
// This attribute is commonly used on structs that directly represent a structure defined by the processor, for example: the Global Descriptor Table
#define ATTRIBUTE_PACKED __attribute__((packed))

// Align the variable to the next address divisible by (same as aligned to) x
#define ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
