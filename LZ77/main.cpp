#include "LZ77.hpp"

int main() {
	LZ77 lz;
	lz.Compress("1.txt");
	lz.UnCompress("2.lzp");
	return 0;
}