#pragma once
#include <string>
#include <fcntl.h>
#include <assert.h>
#include <algorithm>
#include "HaffmanTree.hpp"


/*
* һ���ֽ��ܱ�ʾ�ķ�Χ0-255��������ṹ�壬��¼ÿ���ַ�����Ϣ
* �ַ� + ���ִ��� + Huffman����
*/
struct CharInfo {
	unsigned char _ch; // �ַ�
	size_t _count; // �ַ����ִ���
	std::string _strCode; // �ַ�����
	CharInfo(size_t count = 0) :_count(count) {}
	CharInfo operator+(const CharInfo& c) { return CharInfo(_count + c._count); }
	bool operator>(const CharInfo& c) const { return _count > c._count; }
	bool operator==(const CharInfo& c) const { return _count == c._count; }
};


class FileCompressHuff {
public:
	/*
	* ���캯����_fileInfo�����ʼ����_countÿ���ַ����ֵĴ�ʱ����ʼ��Ϊ0��0��Ϊһ����Чֵ��
	*/
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
		Clear();
		// 1. ͳ��Դ�ļ���ÿ���ַ����ֵĴ���
		FILE* fIn = fopen(path.c_str(), "rb");
		if (fIn == nullptr) { assert(false); return; }

		unsigned char* rBuf = new unsigned char[1024];
		while (true) {
			int rLen = fread(rBuf, 1, 1024, fIn);
			if (rLen == 0) break;
			
			for (int i = 0; i < rLen; ++i)
				_fileInfo[rBuf[i]]._count++;
		}
		
		// 2. ���ַ����ֵĴ���ΪȨֵ����Huffman����CharInfo() ���ɵ���һ����Ч��Ȩֵ
		HuffmanTree<CharInfo> t(_fileInfo, CharInfo());

		// 3. ��ȡÿ���ַ��ı���
		GenerateHuffmanCode(t.getRoot());
		
		// 4. ѹ������ǰ��д��ͷ��Ϣ
		//    �ַ�������(����ͷ��������)���ַ��Լ���Ӧ�Ĵ���(��ԭhuffman��)��Դ�ļ�������
		FILE* fOut = fopen("C:/Users/DELL/Desktop/����/huff.lzp", "wb");
		if (fOut == nullptr) { assert(false); return; }
		
		std::string fType = GetFileType(path) + "\n"; // �ļ�����
		size_t line = 0;	// �ַ�������
		std::string header; // �ַ�:�ַ�����

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

		// 5. �û�ȡ�����ַ��������¸�дԴ�ļ�����ѹ��
		//    ѭ����ȡԴ�ļ��е��ַ������ַ���Ӧ�ı��뱣����ch�У�ch��һ���ֽ���д�����ļ������һ���ֽ����⴦��
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
	* ��ѹ��:�����������⣬һ����fgetsͳ��'\n'ʱ��һ����ͳ��'\0'ʱ
	*/
	void UnCompress(const std::string& path) {
		Clear();
		FILE* fIn = fopen(path.c_str(), "rb");
		// 1. ��ȡͷ��Ϣ
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
		
		// 2. ��ԭHuffman��
		HuffmanTree<CharInfo> t(_fileInfo, CharInfo());

		// 3. ����Huffman����ԭ�ļ�
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
	/*
	* ��ȡ�ļ�����
	*/
	std::string GetFileType(const std::string& fileName) {
		size_t pos = fileName.find_last_of('.');
		return fileName.substr(pos);
	}
	/*
	* _fileInfo��պ���
	*/
	void Clear() {
		for (int i = 0; i < 256; ++i) {
			_fileInfo[i]._ch = i;
			_fileInfo[i]._count = 0;
			_fileInfo[i]._strCode = "";
		}
	}
private:
	std::vector<CharInfo> _fileInfo; // ��256���ַ�����256���ַ�����Ϣ��vector��֯��һ��
};