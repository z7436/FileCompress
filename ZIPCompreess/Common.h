#pragma once

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long long ullong;

// ��һ���ֽ�[0,255]����ʾ��Χ[3,258]
// ���3���ֽ��ظ��滻���258���ֽڽ����滻
const ushort MIN_MATCH = 3;
const ushort MAX_MATCH = 258;
const ushort WSIZE = 32 * 1024; // 32K
