#pragma once

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long long ullong;

// 用一个字节[0,255]来表示范围[3,258]
// 最短3个字节重复替换，最长258个字节进行替换
const ushort MIN_MATCH = 3;
const ushort MAX_MATCH = 258;
const ushort WSIZE = 32 * 1024; // 32K
