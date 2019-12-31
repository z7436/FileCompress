#pragma once
#include <string>
#include <fcntl.h>
#include <assert.h>
#include <algorithm>
#include "HaffmanTree.hpp"


/*
* 一个字节能表示的范围0-255，用这个结构体，记录256个字符的信息
* 字符 + 出现次数 + Huffman编码
*/
struct CharInfo {
	char _ch;
	size_t _count; // 字符出现次数
	std::string _strCode; // 字符编码
	CharInfo(size_t count = 0) :_count(count) {}
	CharInfo operator+(const CharInfo& c) { return CharInfo(_count + c._count); }
	bool operator>(const CharInfo& c) const { return _count > c._count; }
	bool operator==(const CharInfo& c) const { return _count == c._count; }
};


class FileCompressHuff {
public:
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
		// 1. 统计源文件中每个字符出现的次数
		FILE* fIn = fopen(path.c_str(), "r");
		if (fIn == nullptr) { assert(false); return; }

		char* rbuf = new char[1024];
		while (true) {
			int rLen = fread(rbuf, 1, 1024, fIn);
			if (rLen == 0) break;
			
			for (int i = 0; i < rLen; ++i)
				_fileInfo[rbuf[i]]._count++;
		}
		
		// 2. 以字符出现的次数为权值创建Huffman树，CharInfo() 生成的是一个无效的权值
		HuffmanTree<CharInfo> t(_fileInfo, CharInfo());

		// 3. 获取每个字符的编码
		GenerateHuffmanCode(t.getRoot());

		// 4. 用获取到的字符编码重新改写源文件进行压缩
		delete[] rbuf;
		fclose(fIn);
	}
	/*
	* 解压缩
	*/
	void UnCompress(const std::string& path) {}

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
private:
	std::vector<CharInfo> _fileInfo; // 共256个字符，统计256个字符的信息
};