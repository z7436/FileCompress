#include "FileCompressHuff.hpp"

int main() {
	FileCompressHuff F;
	F.Compress("1.pdf");
	F.UnCompress("2.txt");

	return 0;
}