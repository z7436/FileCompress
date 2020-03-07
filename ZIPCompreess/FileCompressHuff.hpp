#pragma once
#include <string>
#include <fcntl.h>
#include <assert.h>
#include <algorithm>
#include "HaffmanTree.hpp"


/*
* 一个字节能表示的范围0-255，用这个结构体，记录每个字符的信息
* 字符 + 出现次数 + Huffman编码
*/
struct CharInfo {
	unsigned char _ch; // 字符
	size_t _count; // 字符出现次数
	std::string _strCode; // 字符编码
	CharInfo(size_t count = 0) :_count(count) {}
	CharInfo operator+(const CharInfo& c) { return CharInfo(_count + c._count); }
	bool operator>(const CharInfo& c) const { return _count > c._count; }
	bool operator==(const CharInfo& c) const { return _count == c._count; }
};


class FileCompressHuff {
public:
	/*
	* 构造函数，_fileInfo数组初始化，_count每个字符出现的此时都初始化为0（0作为一个无效值）
	*/
	FileCompressHuff() :_fileInfo(std::vector<CharInfo>(256)) {
		for (int i = 0; i < 256; ++i) {
			_fileInfo[i]._ch = i;
			_fileInfo[i]._count = 0;
		}
	}
	/*
	* 压缩
	*/
	void Compress(const std::string& path) {
		Clear();
		// 1. 统计源文件中每个字符出现的次数
		FILE* fIn = fopen(path.c_str(), "rb");
		if (fIn == nullptr) { assert(false); return; }

		unsigned char* rBuf = new unsigned char[1024];
		while (true) {
			int rLen = fread(rBuf, 1, 1024, fIn);
			if (rLen == 0) break;
			
			for (int i = 0; i < rLen; ++i)
				_fileInfo[rBuf[i]]._count++;
		}
		
		// 2. 以字符出现的次数为权值创建Huffman树，CharInfo() 生成的是一个无效的权值
		HuffmanTree<CharInfo> t(_fileInfo, CharInfo());

		// 3. 获取每个字符的编码
		GenerateHuffmanCode(t.getRoot());
		
		// 4. 压缩内容前先写入头信息
		//    字符的行数(区分头部和内容)，字符以及对应的次数(还原huffman树)，源文件的类型
		FILE* fOut = fopen("C:/Users/DELL/Desktop/测试/huff.lzp", "wb");
		if (fOut == nullptr) { assert(false); return; }
		
		std::string fType = GetFileType(path) + "\n"; // 文件类型
		size_t line = 0;	// 字符种类数
		std::string header; // 字符:字符数量

		for (size_t i = 0; i < 256; ++i) {
			if (_fileInfo[i]._count == 0) continue;
			line++;
			header += _fileInfo[i]._ch;
			header += ":";
			header += std::to_string(_fileInfo[i]._count);
			header += "\n";
		}
		fwrite(fType.c_str(), 1, fType.size(), fOut);
		std::string sline = std::to_string(line) + "\n";
		fwrite(sline.c_str(), 1, sline.size(), fOut);
		fwrite(header.c_str(), 1, header.size(), fOut);

		// 5. 用获取到的字符编码重新改写源文件进行压缩
		//    循环获取源文件中的字符，将字符对应的编码保存在ch中，ch够一个字节则写入新文件，最后一个字节特殊处理
		fseek(fIn, 0, SEEK_SET);
		unsigned char ch = 0;
		size_t bitCount = 8;
		while (true) {
			size_t rLen = fread(rBuf, 1, 1024, fIn);
			if (rLen == 0) break;
			for (size_t i = 0; i < rLen; ++i) {
				std::string code = _fileInfo[rBuf[i]]._strCode;
				for (size_t j = 0; j < code.size(); ++j) {
					if (code[j] == '1') ch |= 1;
					if (--bitCount == 0) {
						fputc(ch, fOut);
						ch = 0;
						bitCount = 8;
					}
					ch <<= 1;
				}
			}
		}
		if (bitCount != 8) {
			ch <<= bitCount-1;
			fputc(ch, fOut);
		}

		delete[] rBuf;
		fclose(fIn);
		fclose(fOut);
	}
	/*
	* 解压缩:遇到两个问题，一个是fgets统计'\n'时，一个是统计'\0'时
	*/
	void UnCompress(const std::string& path) {
		Clear();
		FILE* fIn = fopen(path.c_str(), "rb");
		// 1. 获取头信息
		std::string fType;
		size_t line = 0;

		char* bufLine = new char[256];
		fType = fgets(bufLine, 256, fIn);
		fType.pop_back();
		line = std::atoi(fgets(bufLine, 256, fIn));

		while (line--) {
			std::string sChar = fgets(bufLine, 256, fIn);
			if (sChar == "\n") sChar += fgets(bufLine, 256, fIn);
			if (sChar == "\0") {
				sChar = bufLine + 1;
				_fileInfo[0]._count = std::stoi(sChar.substr(1));
				continue;
			}
			_fileInfo[(unsigned char)sChar[0]]._count = std::stoi(sChar.substr(2));
		}
		
		// 2. 还原Huffman树
		HuffmanTree<CharInfo> t(_fileInfo, CharInfo());

		// 3. 根据Huffman树还原文件
		FILE* fOut = fopen("3.txt", "wb");
		char* rBuf = new char[1024];
		HuffmanTreeNode<CharInfo>* root = t.getRoot();
		size_t cnt = 8, bitCount = root->_weight._count;
		while (true) {
			size_t rLen = fread(rBuf, 1, 1024, fIn);
			if (rLen == 0) break;
			for (size_t i = 0; i < rLen; ++i) {
				unsigned char ch = rBuf[i];
				while (cnt--) {
					if (ch & 0x80) root = root->_right;
					else root = root->_left;
					if (root->_left == NULL && root->_right == NULL) {
						fputc(root->_weight._ch, fOut);
						root = t.getRoot();
						if (--bitCount == 0) break;
					}
					ch <<= 1;
				}
				cnt = 8;
				if (bitCount == 0) break;
			}
		}


		delete[] bufLine;
		delete[] rBuf;
	}

private:
	/*
	* 根据Huffman树，生成Huffman编码
	* 遍历到叶子节点后利用_parent指针向上查找路径，并记录后反转strCode
	*/
	void GenerateHuffmanCode(HuffmanTreeNode<CharInfo>* root) {
		if (root == NULL) return;
		GenerateHuffmanCode(root->_left);
		GenerateHuffmanCode(root->_right);
		if (root->_left == NULL && root->_right == NULL) {
			std::string strCode;
			for (HuffmanTreeNode<CharInfo> *cur = root; cur->_parent != NULL; cur = cur->_parent) {
				if (cur == cur->_parent->_left) strCode += '0';
				else strCode += '1';
			}
			reverse(strCode.begin(), strCode.end());
			_fileInfo[root->_weight._ch]._strCode = strCode;
		}
	}
	/*
	* 获取文件类型
	*/
	std::string GetFileType(const std::string& fileName) {
		size_t pos = fileName.find_last_of('.');
		return fileName.substr(pos);
	}
	/*
	* _fileInfo清空函数
	*/
	void Clear() {
		for (int i = 0; i < 256; ++i) {
			_fileInfo[i]._ch = i;
			_fileInfo[i]._count = 0;
			_fileInfo[i]._strCode = "";
		}
	}
private:
	std::vector<CharInfo> _fileInfo; // 共256个字符，将256个字符的信息用vector组织在一起
};