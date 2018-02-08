#pragma once
#include "string"
#include <list>
using namespace std;

struct DropStr{
	string name;
	string items;
};


class DropHandle
{
public:
	DropHandle();
	~DropHandle();

	int InitDropList(string dropFile);
	string GetNameByItem(string item);
	string GetItemsByName(string name);
	int DestoryDropList();
	list<DropStr> GetList();

	int InitLimitList(string limitFile);
	int DestoryLimitList();
	int AddLimitItem(string item);
	bool IsLimit(string msg);
	list<string> GetLimitList();

private:
	list<DropStr> m_dropList;
	list<string> m_limitList;
	string m_limitFile;
};

