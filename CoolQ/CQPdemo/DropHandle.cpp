#include "stdafx.h"
#include "iostream"
#include "DropHandle.h"


DropHandle::DropHandle()
{
}


DropHandle::~DropHandle()
{

}

int DropHandle::InitDropList(string dropFile)
{
	//read file
	FILE* fd = fopen(dropFile.c_str(), "r");
	if (fd == NULL)
	{
		return -1;
	}
	char line[2048];
	int lineNum = 0;
	string name = "";
	string items = "";

	while (fgets(line, 2048, fd) != NULL){
		lineNum++;
		if (lineNum % 3 == 1)//name
		{
			name = line;
		}
		else if (lineNum % 3 == 2)//items
		{
			items = line;
		}
		else
		{
			//放入List
			DropStr drop;
			drop.name = name.substr(0, name.length() - 1);
			drop.items = items.substr(0, items.length() - 1);
			m_dropList.push_back(drop);
		}
	}
	fclose(fd);
	return 0;
}

string DropHandle::GetNameByItem(string item)
{
	string result = "";
	bool find = false;

	//您还可以查询类似名字
	//遍历list
	list<DropStr>::iterator i;
	for (i = m_dropList.begin(); i != m_dropList.end(); i++)
	{
		size_t position = i->items.find(item);
		if (position != string::npos)
		{
			result += i->name + ",";
			find = true;
		}
	}

	if (!find)
	{
		result += "无匹配对象！请核对后查找。";
	}
	else
	{
		result = "以下BOSS(物品)均有" + item + "掉落:\r\n" + result;
	}

	return result;
}

string DropHandle::GetItemsByName(string name)
{
	string result = "";
	string similar = "";
	bool find = false;

	//您还可以查询类似名字
	//遍历list
	list<DropStr>::iterator i;
	for (i = m_dropList.begin(); i != m_dropList.end(); i++)
	{
		if (!i->name.compare(0, name.length(), name))
		{
			result = name + "的掉宝是:\r\n" + i->items;
			find = true;
		}
		else
		{
			size_t position = 0;
			do
			{
				position = i->name.find("【");
				if (position != 0)
				{
					break;
				}
				int tail = i->name.find("】");
				if (tail == string::npos)
				{
					break;
				}
				size_t position = i->name.find(name.substr(2, name.length() - 4));
				if (position != string::npos)
				{
					similar += i->name + ",";
				}
			} while (0);
		}
	}

	if (!find)
	{
		result += "无匹配对象！请核对后查找。";
	}
	if (similar != "")
	{
		result += "\r\n您可以查找类匹配串：" + similar;
	}
	return result;
}

int DropHandle::DestoryDropList()
{
	m_dropList.clear();
	return 0;
}

list<DropStr> DropHandle::GetList()
{
	return m_dropList;
}

int DropHandle::InitLimitList(string limitFile)
{
	//read file
	FILE* fd = fopen(limitFile.c_str(), "r");
	if (fd == NULL)
	{
		return -1;
	}
	m_limitFile = limitFile;

	char line[2048];
	int lineNum = 0;

	while (fgets(line, 2048, fd) != NULL){
		string lineStr = line;
		lineStr = lineStr.substr(0, lineStr.length() - 1);
		m_limitList.push_back(lineStr);
	}
	fclose(fd);

	return 0;
}

int DropHandle::DestoryLimitList()
{
	m_limitList.clear();
	return 0;
}

bool DropHandle::IsLimit(string msg)
{
	list<string>::iterator i;
	for (i = m_limitList.begin(); i != m_limitList.end(); i++)
	{
		size_t position = msg.find(i->c_str());
		if (position != string::npos)
		{
			return true;
		}
	}
	return false;
}

int DropHandle::AddLimitItem(string item)
{
	if (item.length() < 8)
	{
		return -2;
	}
	if (IsLimit(item))
	{
		return -1;
	}
	//是否已经存在
	m_limitList.push_back(item);
	FILE* fd = fopen(m_limitFile.c_str(), "a+");
	if (fd == NULL)
	{
		return -1;
	}
	item = "\n" + item;
	fwrite(item.c_str(), 1, item.length(), fd);
	fclose(fd);

	return 0;
}


list<string> DropHandle::GetLimitList()
{
	return m_limitList;
}
