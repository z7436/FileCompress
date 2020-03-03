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
			perror("文件打开失败");
			return;
		}

		// 1. 获取文件大小，如果小于MIN_MATCH则不用进行压缩
		fseek(fIn, 0, SEEK_END);
		ullong fSize = ftell(fIn);
		if (fSize <= MIN_MATCH) {
			std::cerr << "文件太小，不用进行压缩！" << std::endl;
			return;
		}

		// 2. 从文件中读取一个缓冲区的数据到窗口中，lookAhead是先行缓冲区大小
		fseek(fIn, 0, SEEK_SET);
		size_t lookAhead = fread(_pWin, 1, 2 * WSIZE, fIn);
		ushort hashAddr = 0;
		// 由于计算哈希地址的特殊性，因此要提前两次计算来计算起始hashAddr
		for (ushort i = 0; i < MIN_MATCH-1; ++i)
			_ht.HashFunc(hashAddr, _pWin[i]);

		// 3. 进行压缩，start是当前压缩到的位置
		ushort start = 0;

		// 与查找最长匹配相关的变量
		ushort matchHead = 0;		// 匹配头
		ushort curMatchLength = 0;	// 匹配长度
		ushort curMatchDist = 0;	// 匹配距离

		// 与写标记相关的变量
		uchar chFlag = 0;
		uchar bitCount = 0;

		// fOutF:写标记的文件,fOut:压缩后文件，最后要将两个文件组合在一起
		FILE *fOut = fopen("2.lzp", "wb");
		FILE *fOutF = fopen("3.txt", "wb");
		if (fOut == nullptr || fOutF == nullptr) {
			perror("文件打开失败");
			return;
		}
		while (lookAhead) {
			// 1. 将三个字符插入到哈希表中，并获取匹配链的头(第一个重复字符的下标)
			_ht.Insert(hashAddr, _pWin[start + 2], start, matchHead);

			curMatchDist = 0;
			curMatchLength = 0;
			// 2. 验证查找缓冲区中是否找到匹配
			if (matchHead != 0) {
				// 如果找到匹配，则顺着匹配链找到最长匹配
				curMatchLength = LongestMatch(matchHead, curMatchDist, start);
			}

			// 3. 如果找到匹配，将<长度，距离>对写入到压缩文件，否则将当前字符写入文件
			if (curMatchLength >= MIN_MATCH) {
				// 写长度写距离+更新先行缓冲区大小+写标记
				fputc(uchar(curMatchLength-3), fOut);
				fwrite(&curMatchDist, sizeof(curMatchDist), 1, fOut);
				lookAhead -= curMatchLength;
				WriteFlag(fOutF, chFlag, bitCount, true);
				// 将已经匹配的字符串按照三个一组将其插入到哈希表中
				while (--curMatchLength) {
					start++;
					_ht.Insert(hashAddr, _pWin[start+2], start, matchHead);
				}
				start++;
			}
			else {
				// 写当前字符+写标记
				fputc(_pWin[start], fOut);
				WriteFlag(fOutF, chFlag, bitCount, false);
				start++;
				lookAhead--;
			}

			// 检测先行缓冲区中剩余字符个数，小于MIN_LOOKAHEAD需要用新数据填充缓冲区
			if (lookAhead <= MIN_LOOKAHEAD)
				FillWindow(fIn, lookAhead, start);
		}

		// WriteFlag只会将整个字节写入文件，需要将最后不够一个字节进行特殊处理
		if (bitCount > 0 && bitCount < 8) {
			chFlag <<= (8 - bitCount);
			fputc(chFlag, fOutF);
		}

		fclose(fIn);
		fclose(fOutF);

		// 将压缩数据文件和标记文件合并
		// 读取标记信息文件中的内容，写入到压缩文件中
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

		// 删除临时的标记文件
		if (remove("3.txt") != 0)
			std::cerr << "辅助标记文件删除失败！" << std::endl;
	}

	/*
	* 解压缩
	*/
	void UnCompress(const std::string& fPath) {
		// fInD压缩数据指针，fInF标记数据指针
		// fOut还原后的文件wb打开，fR还原后的文件rb打开
		FILE *fInD = fopen(fPath.c_str(), "rb");
		FILE *fInF = fopen(fPath.c_str(), "rb");
		FILE *fOut = fopen("4.txt", "wb");
		FILE *fR = fopen("4.txt", "rb");
		if (fInD == nullptr || fInF == nullptr || fOut == nullptr || fR == nullptr) { 
			std::cerr << "文件打开失败" << std::endl; return; 
		}

		// 从压缩文件末尾获取 源文件大小+标记数据大小
		ullong fileSize = 0;
		size_t flagSize = 0;
		fseek(fInF, 0 - sizeof(fileSize) - sizeof(flagSize), SEEK_END);
		fread(&flagSize, sizeof(flagSize), 1, fInF);
		fread(&fileSize, sizeof(fileSize), 1, fInF);

		// 将标记指针移动到标记数据的起始位置
		fseek(fInF, 0 - sizeof(flagSize) - sizeof(fileSize) - flagSize, SEEK_END);

		// 读取标记文件时相关的变量
		uchar bitCount = 0;
		uchar chFlag = 0;
		ullong encodeCnt = 0;
		while (encodeCnt < fileSize) {
			// bitCount==0 意味着当前字节处理完毕，从标记文件继续读取一个字节的标记
			if (bitCount == 0) {
				chFlag = fgetc(fInF);
				bitCount = 8;
			}

			if (chFlag & 0x80) {
				// 1 代表是一个<长度，距离>对，长度占用一个字节+距离占用两个字节
				ushort matchLen = fgetc(fInD) + 3;
				ushort matchDist = 0;
				fread(&matchDist, sizeof(matchDist), 1, fInD);
				encodeCnt += matchLen;

				// 在读取前文匹配串内容时刷新缓冲区，确保缓冲区中的内容写到文件中
				fflush(fOut);
				// fR:读取前文匹配串中的内容
				fseek(fR, 0 - matchDist, SEEK_END);
				while (matchLen--) {
					uchar ch = fgetc(fR);
					fputc(ch, fOut);

					// 注意：此处刷新缓冲区非常有必要
					// 因为当压缩的时候利用的是两个指针，一个指针指向待压缩位置，一个指针指向匹配起始位置
					// 两个指针同时向后移动直到指向的字符不同停止，那么就有可能出现两个指针指向的区域重叠的情况
					// 例如：abcabcabcabdef
					// 则bcabc(1,7) 和 (5,11)进行了匹配，因为有重叠，还原的时候就必须及时刷新缓冲区
					// 第二个abc不和第一个abc开始匹配是因为第二个abc匹配到第一个abc找到的匹配头是0，而0作为了匹配结束标记
					fflush(fOut);
				}
			}
			else {
				// 0 代表是源字符
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
	* 输出：可能会找到多个匹配结果，对比找到最长的匹配
	* 注意：可能会遇到环装链--解决：设置最大匹配次数 MAX_DIST
	*/
	ushort LongestMatch(ushort matchHead, ushort& MatchDist, ushort start) {
		ushort curMatchLen = 0;
		ushort maxMatchLen = 0;
		uchar maxMatchCount = 255;	// 最大匹配的次数，解决环装链问题
		ushort curMatchStart = 0;	// 当前匹配在查找缓冲区中的位置
		 
		// 查找匹配时查找距离不能太远，即查找距离不能超过MAX_DIST
		ushort limit = start > MAX_DIST ? start - MAX_DIST : 0;
		
		do {
			// 匹配范围：先行缓冲区
			uchar *pstart = _pWin + start;
			uchar *pend = pstart + MAX_MATCH;

			// 查找缓冲区的匹配串的起始
			uchar *pMatchStart = _pWin + matchHead;
			curMatchLen = 0;

			// 可以进行匹配
			while (pstart < pend && *pstart == *pMatchStart) {
				curMatchLen++;
				pstart++;
				pMatchStart++;
			}

			// 保存最佳匹配
			if (curMatchLen > maxMatchLen) {
				maxMatchLen = curMatchLen;
				curMatchStart = matchHead;
			}
		} while ((matchHead = _ht.GetNextHashAddr(matchHead)) > limit && maxMatchCount--);

		MatchDist = start - curMatchStart;
		return maxMatchLen;
	}

	/*
	* 功能：
	*	写标记，解压缩时区分当前字符时源字符，还是<长度，距离>对使用
	* 参数：
	*	ch 用比特位来区分当前字符是源字符还是<长度，距离>对
	*	bitCount 当前字节多少个位已经被设置
	*	isSourceChar 标记当前比特位是源字符还是<长度，距离>对
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
	* 功能：
	*	如果文件大小大于64k则一次并不能压缩全部数据，当先行缓冲区<MIN_LOOKAHEAD时向窗口加载数据
	* 参数：
	*	fIn 带压缩文件的文件指针、lookAhead 传出参数+标记先行缓冲区剩余大小
	*/
	void FillWindow(FILE *fIn, size_t& lookAhead, ushort& start) {
		// 如果start<WSIZE说明压缩已经进行到了右窗（因为已经压缩到了文件的最后，此时pWin窗口只有不到一半的数据）
		// 这种情况就不用再填充窗口，否则将右窗的数据覆盖到左窗将会出错
		if (start >= WSIZE) {
			// 1. 将右窗数据搬移到左窗
			memcpy(_pWin, _pWin + WSIZE, WSIZE);
			memset(_pWin + WSIZE, 0, WSIZE);
			start -= WSIZE;
		
			// 2. 更新哈希表
			_ht.update();

			// 3. 向右窗中加载数据
			if (!feof(fIn))
				lookAhead += fread(_pWin + WSIZE, 1, WSIZE, fIn);
		}
	}
private:
	uchar *_pWin; // 带压缩数据缓冲区
	LZHashTable _ht;
};