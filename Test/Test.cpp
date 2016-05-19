// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <300Utils.h>
#include <iostream>

int main()
{
	int pack = OpenPack(LR"(D:\Program Files (x86)\《300英雄》\Data.jmp)");
	int iter = CreateIterator(pack);
	do
	{
		std::cout << GetFileName(iter) << std::endl;
	} while (MoveToNextFile(iter));
	CloseIterator(iter);
	ClosePack(pack);

	system("pause");
    return 0;
}

