#pragma once
#include <string.h>
#include "Common.h"


const ushort HASH_BITS = 15;			// ��ϣ��ַռ��15������λ
const ushort HASH_SIZE = (1 << 15);		// ��ϣ��2^15��Ͱ
const ushort HASH_MASK = HASH_SIZE - 1;	// ��ϣ���룬��������ϣ��ͻ��ʱ��Խ��

/*
* ��ϣ��
*	һ��2^16��λ�ã���һ��������Ϊ��ϣͰ(��ϣ��ַ15��bitλ��ʾ)��ǰһ�����������ϣ��ͻ
*	��������СΪ64k����˹�ϣ���е�Ԫ���������ֽ�(ushort)���洢����˹�ϣ��ռ�ÿռ�Ϊ(2^16)*2=128k
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
	* ���ϣ���в���Ԫ��
	* ����
	*	hashAddr �ϴεĹ�ϣ��ַ��ch ����ƥ�������ַ������һ��
	*	pos ch�ڻ������ڵ�λ�ã�matchHead ���ƥ���򱣴�ƥ�䴮����ʼλ��
	*/
	void Insert(ushort& hashAddr, uchar ch, ushort pos, ushort& matchHead) {
		HashFunc(hashAddr, ch);

		// pos & HASH_MASK��֤��Խ��
		_prev[pos & HASH_MASK] = _head[hashAddr];
		matchHead = _head[hashAddr];
		_head[hashAddr] = pos;
	}

	/*
	* ��ϣ���������������ֽڼ����ϣ��ַ
	*	A(4,5) + A(6,7,8) ^ B(1,2,3) + B(4,5) + B(6,7,8) ^ C(1,2,3) + C(4,5,6,7,8)
	*	A B C �ֱ�Ϊ��һ�ڶ��������ֽڣ�A(4,5) ָ��һ���ֽڵĵ� 4,5 λ��������
	*	^ �����+ ������
	*	ÿ�����ֵ h ������ ((ǰ1��h << 5) ^ c)ȡ��15λ
	*	��˱��εĹ�ϣ��ַ����ǰһ�ι�ϣ��ַ�����ϣ��ٽ�ϵ�ǰ�ַ�ch���������
	* �������壺
	*	hashAddr ��һ�εĹ�ϣ��ַ��ch ��ǰ�ַ�
	* HASH_MASKΪWSIZE-1��&��������Ҫ��Ϊ�˷�ֹ��ϣ��ַԽ��
	*/
	void HashFunc(ushort& hashAddr, uchar ch) {
		hashAddr = (((hashAddr) << H_SHIFT()) ^ (ch)) & HASH_MASK;
	}

	/*
	* ��ȡ���е��¸���ϣ��ַ
	*/
	ushort GetNextHashAddr(ushort matchHead) {
		return _prev[matchHead	& HASH_MASK];
	}

	/*
	* ���¹�ϣ�����򴰿������¼������ݺ���Ҫ���¹�ϣ��
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
	* ������ϣ����ʹ��
	*/
	ushort H_SHIFT() {
		return (HASH_BITS + MIN_MATCH - 1) / MIN_MATCH;
	}
private:
	ushort *_prev;
	ushort *_head;
};