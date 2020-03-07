#pragma once
#include <string.h>
#include "Common.h"


const ushort HASH_BITS = 15;			// 哈希地址占用15个比特位
const ushort HASH_SIZE = (1 << 15);		// 哈希表共2^15个桶
const ushort HASH_MASK = HASH_SIZE - 1;	// 哈希掩码，避免解决哈希冲突的时候越界

/*
* 哈希表
*	一共2^16个位置，后一半用来作为哈希桶(哈希地址15个bit位表示)，前一半用来解决哈希冲突
*	缓冲区大小为64k，因此哈希表中的元素用两个字节(ushort)来存储，因此哈希表占用空间为(2^16)*2=128k
*/
class LZHashTable {
public:
	LZHashTable(ushort size)
		: _prev(new ushort[2 * size])
		, _head(_prev + size)
	{
		memset(_prev, 0, 2 * size * sizeof(ushort));
	}
	~LZHashTable() {
		delete[] _prev;
		_prev = nullptr;
		_head = nullptr;
	}
	/*
	* 向哈希表中插入元素
	* 参数
	*	hashAddr 上次的哈希地址、ch 本次匹配三个字符的最后一个
	*	pos ch在滑动窗口的位置，matchHead 如果匹配则保存匹配串的起始位置
	*/
	void Insert(ushort& hashAddr, uchar ch, ushort pos, ushort& matchHead) {
		HashFunc(hashAddr, ch);

		// pos & HASH_MASK保证不越界
		_prev[pos & HASH_MASK] = _head[hashAddr];
		matchHead = _head[hashAddr];
		_head[hashAddr] = pos;
	}

	/*
	* 哈希函数：根据三个字节计算哈希地址
	*	A(4,5) + A(6,7,8) ^ B(1,2,3) + B(4,5) + B(6,7,8) ^ C(1,2,3) + C(4,5,6,7,8)
	*	A B C 分别为第一第二第三个字节，A(4,5) 指第一个字节的第 4,5 位二进制码
	*	^ 是异或，+ 是连接
	*	每个结果值 h 都等于 ((前1个h << 5) ^ c)取右15位
	*	因此本次的哈希地址是在前一次哈希地址基础上，再结合当前字符ch计算出来的
	* 参数含义：
	*	hashAddr 上一次的哈希地址，ch 当前字符
	* HASH_MASK为WSIZE-1，&上掩码主要是为了防止哈希地址越界
	*/
	void HashFunc(ushort& hashAddr, uchar ch) {
		hashAddr = (((hashAddr) << H_SHIFT()) ^ (ch)) & HASH_MASK;
	}

	/*
	* 获取链中的下个哈希地址
	*/
	ushort GetNextHashAddr(ushort matchHead) {
		return _prev[matchHead	& HASH_MASK];
	}

	/*
	* 更新哈希表，当向窗口中重新加载数据后需要更新哈希表
	*/
	void update() {
		for (ushort i = 0; i < WSIZE; ++i) {
			if (_head[i] >= WSIZE)
				_head[i] -= WSIZE;
			else
				_head[i] = 0;

			if (_prev[i] >= WSIZE)
				_prev[i] -= WSIZE;
			else
				_prev[i] = 0;
		}
	}
private:
	/*
	* 辅助哈希函数使用
	*/
	ushort H_SHIFT() {
		return (HASH_BITS + MIN_MATCH - 1) / MIN_MATCH;
	}
private:
	ushort *_prev;
	ushort *_head;
};