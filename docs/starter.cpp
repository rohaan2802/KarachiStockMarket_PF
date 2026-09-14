#include <iostream>
#include <ctime>
#include <conio.h>
using namespace std;
void redrawMarket();

int main()
{
	char ch;
	srand(time(0));
	while(ch!=27)
	{
		redrawMarket();
		ch = getch();
		system("clear");
	}
}

void redrawMarket()
{
	int pso,hbl;
	cout<<"SYMBOL\t"<<"Price"<<endl;
	pso = rand();
	hbl = rand();
	cout<<"PSO\t"<<pso<<endl;
	cout<<"HBL\t"<<hbl<<endl;
}

