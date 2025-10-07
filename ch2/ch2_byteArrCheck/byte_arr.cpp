#include <iostream>
using namespace std;

enum EndianType
{
	BIG_ENDIAN, LITTLE_ENDIAN
};

EndianType checkEndian()
{
	int value = 0x12345678;
	char* p = reinterpret_cast<char*>(& value);

	if (*p == 0x78)
	{
		return LITTLE_ENDIAN;
	}
	else
	{
		return BIG_ENDIAN;
	}
}

void printEndian()
{
	if (checkEndian() == BIG_ENDIAN)
	{
		cout << "호스트 바이트 정렬 방식 : BIG ENDIAN\n";
	}
	else
	{
		cout << "호스트 바이트 정렬 방식 : LITTLE ENDIAN\n";
	}
}

int main()
{
	printEndian();
	return 0;
}


