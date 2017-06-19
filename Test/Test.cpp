// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <300Utils.h>
#include <iostream>
#include <vector>
#include "natStream.h"

using namespace NatsuLib;

int main()
{
	int pack = OpenPack(LR"(D:\Program Files (x86)\《300英雄》\Data.jmp)");
	int iter = CreateIterator(pack);
	do
	{
		if (strcmp(GetFileName(iter), "..\\data\\splash00.bmp") == 0)
		{
			break;
		}
	} while (MoveToNextFile(iter));

	natFileStream fs(LR"(D:\Visual Studio 2015\Projects\300Test\Release\data\splash00.bmp)", true, false);
	std::vector<byte> content(fs.GetSize());
	fs.ReadBytes(content.data(), content.size());

	SetFileContent(iter, content.data(), content.size());

	CloseIterator(iter);
	ClosePack(pack);

	system("pause");
	return 0;
}
