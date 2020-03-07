#include <chrono>
#include "FileCompressHuff.hpp"
#include "LZ77.hpp"

int main() {
	auto start = std::chrono::steady_clock::now();

	LZ77 lz;
	FileCompressHuff huff;

	// lz.Compress("C:/Users/DELL/Desktop/����/1569068299.MP4");
	huff.Compress("C:/Users/DELL/Desktop/����/IMG_2745.MP4");

	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double, std::micro> elapsed = end - start;// std::micro ��ʾ��΢��Ϊʱ�䵥λ
	std::cout << "time: " << elapsed.count() << std::endl;

	return 0;
}