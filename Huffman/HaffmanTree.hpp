#pragma once
#include <stdio.h>
#include <vector>
#include <queue>

/*
* Huffman树节点，_weight为权值
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
* 自定义priority_queue的比较规则，目的为了建一个小堆
*/
template<class W>
struct Cmp {
	typedef HuffmanTreeNode<W> Node;
	bool operator()(const Node* left, const Node* right) {
		return left->_weight > right->_weight;
	}
};


/*
* Huffman树
*/
template<class W>
class HuffmanTree {
	typedef HuffmanTreeNode<W> Node;
public:
	HuffmanTree() :_root(NULL) {}
	HuffmanTree(const std::vector<W> v, W w) { Create(v, w); }
	~HuffmanTree() { _Destory(_root); }
	/*
	* 创建Huffman树，invalid是一个无效的权值，用来对 v 做筛选
	*/
	void Create(const std::vector<W>& v, const W& invalid) {
		// 1.构建森林
		std::priority_queue<Node*, std::vector<Node*>, Cmp<W>> q;
		for (auto e : v) {
			if (e == invalid) continue; // 权值为无效权值，不需要添加
			q.push(new Node(e));
		}
		// 2.森林中树大于两个，取出权值最小的两个树，以他们为左右子树创建新树，权值为左右子树之和
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
		// 3.最终剩下的树就是Huffman树
		_root = q.top();
	}
	/*
	* 获取树的根节点
	*/
	Node* getRoot() {
		return _root;
	}
private:
	/*
	* 销毁Huffman树
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
