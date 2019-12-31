#pragma once
#include <stdio.h>
#include <vector>
#include <queue>

/*
* Huffman���ڵ㣬_weightΪȨֵ
*/
template<class W>
struct HuffmanTreeNode {
	HuffmanTreeNode(const W& weight = W())
		:_left(NULL),_right(NULL),_parent(NULL),_weight(weight) {}
	HuffmanTreeNode<W>* _left;
	HuffmanTreeNode<W>* _right;
	HuffmanTreeNode<W>* _parent;
	W _weight;
};

/*
* �Զ���priority_queue�ıȽϹ���Ŀ��Ϊ�˽�һ��С��
*/
template<class W>
struct Cmp {
	typedef HuffmanTreeNode<W> Node;
	bool operator()(const Node* left, const Node* right) {
		return left->_weight > right->_weight;
	}
};


/*
* Huffman��
*/
template<class W>
class HuffmanTree {
	typedef HuffmanTreeNode<W> Node;
public:
	HuffmanTree() :_root(NULL) {}
	HuffmanTree(const std::vector<W> v, W w) { Create(v, w); }
	~HuffmanTree() { _Destory(_root); }
	/*
	* ����Huffman����invalid��һ����Ч��Ȩֵ�������� v ��ɸѡ
	*/
	void Create(const std::vector<W>& v, const W& invalid) {
		// 1.����ɭ��
		std::priority_queue<Node*, std::vector<Node*>, Cmp<W>> q;
		for (auto e : v) {
			if (e == invalid) continue; // ȨֵΪ��ЧȨֵ������Ҫ���
			q.push(new Node(e));
		}
		// 2.ɭ����������������ȡ��Ȩֵ��С����������������Ϊ������������������ȨֵΪ��������֮��
		int qLen = q.size();
		while (qLen > 1) {
			Node* left = q.top();
			q.pop();
			Node* right = q.top();
			q.pop();
			Node* nTree = new Node(left->_weight + right->_weight);
			nTree->_left = left;
			nTree->_right = right;
			left->_parent = nTree;
			right->_parent = nTree;
			q.push(nTree);
			qLen = q.size();
		}
		// 3.����ʣ�µ�������Huffman��
		_root = q.top();
	}
	/*
	* ��ȡ���ĸ��ڵ�
	*/
	Node* getRoot() {
		return _root;
	}
private:
	/*
	* ����Huffman��
	*/
	void _Destory(Node*& root) {
		if (root == NULL) return;
		_Destory(root->_left);
		_Destory(root->_right);
		delete root;
		root = NULL;
	}
private:
	Node* _root;
};
