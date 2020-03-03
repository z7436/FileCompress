#pragma once

#include <string>
#include <iostream>
#include "LZHashTable.hpp"

const ushort MIN_LOOKAHEAD = MAX_MATCH + MIN_MATCH + 1;
const ushort MAX_DIST = WSIZE - MIN_LOOKAHEAD;

class LZ77 {
public:
	LZ77()
		:_pWin(new uchar[WSIZE*2])
		, _ht(WSIZE)
	{}
	~LZ77() {
		delete[] _pWin;
		_pWin = nullptr;
	}

public:
	void Compress(const std::string& fPath) {
		FILE *fIn = fopen(fPath.c_str(), "rb");
		if (fIn == nullptr) {
			perror("�ļ���ʧ��");
			return;
		}

		// 1. ��ȡ�ļ���С�����С��MIN_MATCH���ý���ѹ��
		fseek(fIn, 0, SEEK_END);
		ullong fSize = ftell(fIn);
		if (fSize <= MIN_MATCH) {
			std::cerr << "�ļ�̫С�����ý���ѹ����" << std::endl;
			return;
		}

		// 2. ���ļ��ж�ȡһ�������������ݵ������У�lookAhead�����л�������С
		fseek(fIn, 0, SEEK_SET);
		size_t lookAhead = fread(_pWin, 1, 2 * WSIZE, fIn);
		ushort hashAddr = 0;
		// ���ڼ����ϣ��ַ�������ԣ����Ҫ��ǰ���μ�����������ʼhashAddr
		for (ushort i = 0; i < MIN_MATCH-1; ++i)
			_ht.HashFunc(hashAddr, _pWin[i]);

		// 3. ����ѹ����start�ǵ�ǰѹ������λ��
		ushort start = 0;

		// ������ƥ����صı���
		ushort matchHead = 0;		// ƥ��ͷ
		ushort curMatchLength = 0;	// ƥ�䳤��
		ushort curMatchDist = 0;	// ƥ�����

		// ��д�����صı���
		uchar chFlag = 0;
		uchar bitCount = 0;

		// fOutF:д��ǵ��ļ�,fOut:ѹ�����ļ������Ҫ�������ļ������һ��
		FILE *fOut = fopen("2.lzp", "wb");
		FILE *fOutF = fopen("3.txt", "wb");
		if (fOut == nullptr || fOutF == nullptr) {
			perror("�ļ���ʧ��");
			return;
		}
		while (lookAhead) {
			// 1. �������ַ����뵽��ϣ���У�����ȡƥ������ͷ(��һ���ظ��ַ����±�)
			_ht.Insert(hashAddr, _pWin[start + 2], start, matchHead);

			curMatchDist = 0;
			curMatchLength = 0;
			// 2. ��֤���һ��������Ƿ��ҵ�ƥ��
			if (matchHead != 0) {
				// ����ҵ�ƥ�䣬��˳��ƥ�����ҵ��ƥ��
				curMatchLength = LongestMatch(matchHead, curMatchDist, start);
			}

			// 3. ����ҵ�ƥ�䣬��<���ȣ�����>��д�뵽ѹ���ļ������򽫵�ǰ�ַ�д���ļ�
			if (curMatchLength >= MIN_MATCH) {
				// д����д����+�������л�������С+д���
				fputc(uchar(curMatchLength-3), fOut);
				fwrite(&curMatchDist, sizeof(curMatchDist), 1, fOut);
				lookAhead -= curMatchLength;
				WriteFlag(fOutF, chFlag, bitCount, true);
				// ���Ѿ�ƥ����ַ�����������һ�齫����뵽��ϣ����
				while (--curMatchLength) {
					start++;
					_ht.Insert(hashAddr, _pWin[start+2], start, matchHead);
				}
				start++;
			}
			else {
				// д��ǰ�ַ�+д���
				fputc(_pWin[start], fOut);
				WriteFlag(fOutF, chFlag, bitCount, false);
				start++;
				lookAhead--;
			}

			// ������л�������ʣ���ַ�������С��MIN_LOOKAHEAD��Ҫ����������仺����
			if (lookAhead <= MIN_LOOKAHEAD)
				FillWindow(fIn, lookAhead, start);
		}

		// WriteFlagֻ�Ὣ�����ֽ�д���ļ�����Ҫ����󲻹�һ���ֽڽ������⴦��
		if (bitCount > 0 && bitCount < 8) {
			chFlag <<= (8 - bitCount);
			fputc(chFlag, fOutF);
		}

		fclose(fIn);
		fclose(fOutF);

		// ��ѹ�������ļ��ͱ���ļ��ϲ�
		// ��ȡ�����Ϣ�ļ��е����ݣ�д�뵽ѹ���ļ���
		FILE *fInF = fopen("3.txt", "rb");
		size_t flagSize = 0;
		uchar *pReadBuff = new uchar[1024];
		while (true) {
			size_t rdSize = fread(pReadBuff, 1, 1024, fInF);
			if (rdSize == 0) break;
			fwrite(pReadBuff, 1, rdSize, fOut);
			flagSize += rdSize;
		}
		fwrite(&flagSize, 1, sizeof(flagSize), fOut);
		fwrite(&fSize, 1, sizeof(fSize), fOut);
		fclose(fInF);
		fclose(fOut);
		delete[] pReadBuff;

		// ɾ����ʱ�ı���ļ�
		if (remove("3.txt") != 0)
			std::cerr << "��������ļ�ɾ��ʧ�ܣ�" << std::endl;
	}

	/*
	* ��ѹ��
	*/
	void UnCompress(const std::string& fPath) {
		// fInDѹ������ָ�룬fInF�������ָ��
		// fOut��ԭ����ļ�wb�򿪣�fR��ԭ����ļ�rb��
		FILE *fInD = fopen(fPath.c_str(), "rb");
		FILE *fInF = fopen(fPath.c_str(), "rb");
		FILE *fOut = fopen("4.txt", "wb");
		FILE *fR = fopen("4.txt", "rb");
		if (fInD == nullptr || fInF == nullptr || fOut == nullptr || fR == nullptr) { 
			std::cerr << "�ļ���ʧ��" << std::endl; return; 
		}

		// ��ѹ���ļ�ĩβ��ȡ Դ�ļ���С+������ݴ�С
		ullong fileSize = 0;
		size_t flagSize = 0;
		fseek(fInF, 0 - sizeof(fileSize) - sizeof(flagSize), SEEK_END);
		fread(&flagSize, sizeof(flagSize), 1, fInF);
		fread(&fileSize, sizeof(fileSize), 1, fInF);

		// �����ָ���ƶ���������ݵ���ʼλ��
		fseek(fInF, 0 - sizeof(flagSize) - sizeof(fileSize) - flagSize, SEEK_END);

		// ��ȡ����ļ�ʱ��صı���
		uchar bitCount = 0;
		uchar chFlag = 0;
		ullong encodeCnt = 0;
		while (encodeCnt < fileSize) {
			// bitCount==0 ��ζ�ŵ�ǰ�ֽڴ�����ϣ��ӱ���ļ�������ȡһ���ֽڵı��
			if (bitCount == 0) {
				chFlag = fgetc(fInF);
				bitCount = 8;
			}

			if (chFlag & 0x80) {
				// 1 ������һ��<���ȣ�����>�ԣ�����ռ��һ���ֽ�+����ռ�������ֽ�
				ushort matchLen = fgetc(fInD) + 3;
				ushort matchDist = 0;
				fread(&matchDist, sizeof(matchDist), 1, fInD);
				encodeCnt += matchLen;

				// �ڶ�ȡǰ��ƥ�䴮����ʱˢ�»�������ȷ���������е�����д���ļ���
				fflush(fOut);
				// fR:��ȡǰ��ƥ�䴮�е�����
				fseek(fR, 0 - matchDist, SEEK_END);
				while (matchLen--) {
					uchar ch = fgetc(fR);
					fputc(ch, fOut);

					// ע�⣺�˴�ˢ�»������ǳ��б�Ҫ
					// ��Ϊ��ѹ����ʱ�����õ�������ָ�룬һ��ָ��ָ���ѹ��λ�ã�һ��ָ��ָ��ƥ����ʼλ��
					// ����ָ��ͬʱ����ƶ�ֱ��ָ����ַ���ֹͬͣ����ô���п��ܳ�������ָ��ָ��������ص������
					// ���磺abcabcabcabdef
					// ��bcabc(1,7) �� (5,11)������ƥ�䣬��Ϊ���ص�����ԭ��ʱ��ͱ��뼰ʱˢ�»�����
					// �ڶ���abc���͵�һ��abc��ʼƥ������Ϊ�ڶ���abcƥ�䵽��һ��abc�ҵ���ƥ��ͷ��0����0��Ϊ��ƥ��������
					fflush(fOut);
				}
			}
			else {
				// 0 ������Դ�ַ�
				uchar ch = fgetc(fInD);
				fputc(ch, fOut);
				encodeCnt++;
			}

			chFlag <<= 1;
			bitCount--;
		}

		fclose(fInD);
		fclose(fInF);
		fclose(fOut);
		fclose(fR);
	}

private:
	/*
	* ��������ܻ��ҵ����ƥ�������Ա��ҵ����ƥ��
	* ע�⣺���ܻ�������װ��--������������ƥ����� MAX_DIST
	*/
	ushort LongestMatch(ushort matchHead, ushort& MatchDist, ushort start) {
		ushort curMatchLen = 0;
		ushort maxMatchLen = 0;
		uchar maxMatchCount = 255;	// ���ƥ��Ĵ����������װ������
		ushort curMatchStart = 0;	// ��ǰƥ���ڲ��һ������е�λ��
		 
		// ����ƥ��ʱ���Ҿ��벻��̫Զ�������Ҿ��벻�ܳ���MAX_DIST
		ushort limit = start > MAX_DIST ? start - MAX_DIST : 0;
		
		do {
			// ƥ�䷶Χ�����л�����
			uchar *pstart = _pWin + start;
			uchar *pend = pstart + MAX_MATCH;

			// ���һ�������ƥ�䴮����ʼ
			uchar *pMatchStart = _pWin + matchHead;
			curMatchLen = 0;

			// ���Խ���ƥ��
			while (pstart < pend && *pstart == *pMatchStart) {
				curMatchLen++;
				pstart++;
				pMatchStart++;
			}

			// �������ƥ��
			if (curMatchLen > maxMatchLen) {
				maxMatchLen = curMatchLen;
				curMatchStart = matchHead;
			}
		} while ((matchHead = _ht.GetNextHashAddr(matchHead)) > limit && maxMatchCount--);

		MatchDist = start - curMatchStart;
		return maxMatchLen;
	}

	/*
	* ���ܣ�
	*	д��ǣ���ѹ��ʱ���ֵ�ǰ�ַ�ʱԴ�ַ�������<���ȣ�����>��ʹ��
	* ������
	*	ch �ñ���λ�����ֵ�ǰ�ַ���Դ�ַ�����<���ȣ�����>��
	*	bitCount ��ǰ�ֽڶ��ٸ�λ�Ѿ�������
	*	isSourceChar ��ǵ�ǰ����λ��Դ�ַ�����<���ȣ�����>��
	*/
	void WriteFlag(FILE *fOut, uchar& chFlag, uchar& bitCount, bool isLenPair) {
		chFlag <<= 1;
		if (isLenPair) chFlag |= 1;
		bitCount++;
		if (bitCount == 8) {
			fputc(chFlag, fOut);
			chFlag = 0;
			bitCount = 0;
		}
	}

	/*
	* ���ܣ�
	*	����ļ���С����64k��һ�β�����ѹ��ȫ�����ݣ������л�����<MIN_LOOKAHEADʱ�򴰿ڼ�������
	* ������
	*	fIn ��ѹ���ļ����ļ�ָ�롢lookAhead ��������+������л�����ʣ���С
	*/
	void FillWindow(FILE *fIn, size_t& lookAhead, ushort& start) {
		// ���start<WSIZE˵��ѹ���Ѿ����е����Ҵ�����Ϊ�Ѿ�ѹ�������ļ�����󣬴�ʱpWin����ֻ�в���һ������ݣ�
		// ��������Ͳ�������䴰�ڣ������Ҵ������ݸ��ǵ��󴰽������
		if (start >= WSIZE) {
			// 1. ���Ҵ����ݰ��Ƶ���
			memcpy(_pWin, _pWin + WSIZE, WSIZE);
			memset(_pWin + WSIZE, 0, WSIZE);
			start -= WSIZE;
		
			// 2. ���¹�ϣ��
			_ht.update();

			// 3. ���Ҵ��м�������
			if (!feof(fIn))
				lookAhead += fread(_pWin + WSIZE, 1, WSIZE, fIn);
		}
	}
private:
	uchar *_pWin; // ��ѹ�����ݻ�����
	LZHashTable _ht;
};