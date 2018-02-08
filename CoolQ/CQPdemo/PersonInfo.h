#ifndef QQROBOT_PERSON_INFO_H
#define  QQROBOT_PERSON_INFO_H
#include <string>
using namespace std;

class PersonInfo {
protected:
	int location;
	string decodedInfo;
	void parseQQ();
	void parseNickName();
	void parseSex();
	void parseAge();
	int parseInt();
	int parseShortInt();
	string parseString();
	int32_t parseLongInt();
public:
	int64_t qq;
	int sex;//0ÄÐ1Å®255Î´Öª
	int age;
	string nickName;
	PersonInfo();
	PersonInfo(const string& infoString);
	~PersonInfo();
	void parse(const char* infoString);
	void parse(const string& infoString);
	void show();
};
#endif // !QQROBOT_PERSON_INFO_H
