#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <math.h>
#include <vector>
#include <emmintrin.h>
#include <queue>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

void Reverse(char* s) {
	size_t len = strlen(s);
	size_t halfway = strlen(s) / 2;
	for (size_t i = 0; i < halfway; ++i) {
		char temp = s[i];
		s[i] = s[len - 1 - i];
		s[len - 1 - i] = temp;
	}
}

inline int FirstSetBit(int num) {
	int pos = 1;
	for (int i = 0; i < 16; i++) {
		if (!(num & (1 << (16 - i))))
			pos++;
		else
			break;
	}
	return pos;
}

void Die(const char *msg) {
	fprintf(stderr, "oops: %s\n", msg);
	exit(1);
}

struct Entry
{
	unsigned int prio;
	char data[16];
};

bool operator < (const Entry &a, const Entry &b)
{
	if (a.prio != b.prio)
		return a.prio > b.prio;
	return strcmp(a.data, b.data) > 0;
}

inline bool issp(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}


int main(int argc, char **argv) {
	int fd = open(argv[1], O_RDONLY, 0);
	if (fd < 0) {
		Die("open failed");
	}

	struct stat st;
	if (fstat(fd, &st) < 0) {
		Die("failed to stat file");
	}

	size_t fsize = (size_t)st.st_size;
	char* fmap = (char*)mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
	if (fmap == MAP_FAILED) {
		Die("failed to map the file");
	}

	char* strt = fmap;
	char* p = fmap + fsize - 1;

	//to lookup offset to set p to in O(1)
	std::vector<int> lowest_set_bit(pow(2, 16) + 1);
	for (int i = 0; i < lowest_set_bit.size(); ++i) {
		lowest_set_bit[i] = FirstSetBit(i);
	}

	Entry e;
	unsigned mul = 1;
	unsigned int v;
	
	const char* b = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"; //somewhy works faster than set1_epi8
	
	std::priority_queue<Entry> chart;
	bool chart_filled = false;
	for (;;)
	{
		while (issp(*p) && p > strt)
			p--;
		if (p <= strt)
			break;
		if (*p > '9' || *p < '0')
			Die("format error: number expected");

		v = 0;
		mul = 1;
		while (*p <= '9' && *p >= '0') {
			v = v + ((*p) - '0') * mul;
			p--;
			mul *= 10;
		}
		
		//skipping everything till next numbers' start
		if (chart_filled && chart.top().prio >= v) {
			uint32_t mask = 0;
			if (p - strt > 16) {
				for (; mask == 0; p -= 16) {
					__m128i x = _mm_loadu_si128((__m128i*)(p - 16));
					__m128i y = _mm_loadu_si128((__m128i*)b);
					__m128i vcmp = _mm_cmpeq_epi8(x, y);
					mask = _mm_movemask_epi8(vcmp);
					if (mask != 0)
						break;
				}
			}
			else {
				break;
			}
			p -= lowest_set_bit[mask];
			continue;
		}

		while (issp(*p) && p > strt)
			p--;
		if (p <= strt)
			break;

		if (*p-- != ',') {
			Die("format error: comma expected");
		}

		e.prio = v;
		int t;
		for (t = 0; t < 16 && (((*p | 0x20) >= 'a' && (*p | 0x20) <= 'z') || (*p <= '9' && *p >= '0')) && *p != ',' && p > strt; t++, p--)
			e.data[t] = *p;
		if (t == 16 && p > strt && !issp(*p))
			Die("format error: ident too long");
		e.data[t] = '\0';
		Reverse(e.data);
		if (chart.size() == 100) {
			chart.pop();
			chart_filled = true;
		}
		chart.push(e);
	}

	FILE *fp2 = fopen(argv[2], "wb+");
	if (!fp2)
		Die("write failed");
	while (!chart.empty()) {
		fprintf(fp2, "%s, %u\n", chart.top().data, chart.top().prio);
		chart.pop();
	}

	close(fd);
	munmap(fmap, fsize);
	fclose(fp2);
}

