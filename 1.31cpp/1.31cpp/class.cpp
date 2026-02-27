#include <iostream>
using namespace std;
class Queue
{
public:
	void push(int x);
private:
	int* a;
	int top;
	int capacity;
};

struct Date
{
public:
	void Init(int year, int month, int day);

	int _year;
	int _month;;
	int _day;
};




typedef struct ListnodeC
{
	int val;
	struct ListnodeC* node;
};



struct ListnodeCPP
{
	int val;
	ListnodeCPP* node;
};

int main()
{
	Date d2;
	Date d3;
	d2.Init(1995, 3, 16);
	struct Date d1;
	return 0;
}