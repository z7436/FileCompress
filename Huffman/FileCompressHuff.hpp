#pragma once
#include <string>
#include <fcntl.h>
#include <assert.h>
#include <algorithm>
#include "HaffmanTree.hpp"


/*
* һ���ֽ��ܱ�ʾ�ķ�Χ0-255��������ṹ�壬��¼256���ַ�����Ϣ
* �ַ� + ���ִ��� + Huffman����
*/
struct CharInfo {
	char _ch;
	size_t _count; // �ַ����ִ���
	std::string _strCode; // �ַ�����
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
	* ѹ��
	*/
	void Compress(const std::string& path) {
		// 1. ͳ��Դ�ļ���ÿ���ַ����ֵĴ���
		FILE* fIn = fopen(path.c_str(), "r");
		if (fIn == nullptr) { assert(false); return; }

		char* rbuf = new char[1024];
		while (true) {
			int rLen = fread(rbuf, 1, 1024, fIn);
			if (rLen == 0) break;
			
			for (int i = 0; i < rLen; ++i)
				_fileInfo[rbuf[i]]._count++;
		}
		
		// 2. ���ַ����ֵĴ���ΪȨֵ����Huffman����CharInfo() ���ɵ���һ����Ч��Ȩֵ
		HuffmanTree<CharInfo> t(_fileInfo, CharInfo());

		// 3. ��ȡÿ���ַ��ı���
		GenerateHuffmanCode(t.getRoot());

		// 4. �û�ȡ�����ַ��������¸�дԴ�ļ�����ѹ��
		delete[] rbuf;
		fclose(fIn);
	}
	/*
	* ��ѹ��
	*/
	void UnCompress(const std::string& path) {}

private:
	/*
	* ����Huffman��������Huffman����
	* ������Ҷ�ӽڵ������_parentָ�����ϲ���·��������¼��תstrCode
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
	std::vector<CharInfo> _fileInfo; // ��256���ַ���ͳ��256���ַ�����Ϣ
};