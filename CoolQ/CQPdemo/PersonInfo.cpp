#include "stdafx.h"
#include "PersonInfo.h"
#include "Util.h"

PersonInfo::PersonInfo() {
}
PersonInfo::PersonInfo(const string& infoString) {
	parse(infoString);
}

PersonInfo::~PersonInfo() {

}
void PersonInfo::parse(const char * infoString) {
	parse(string(infoString));
}

void PersonInfo::parse(const string& infoString) {

	decodedInfo = Util::base64_decode(infoString);
	location = 0;
	parseQQ();
	parseNickName();
	parseSex();
	parseAge();
}

void PersonInfo::parseQQ() {
	qq = parseLongInt();
}

void PersonInfo::parseNickName() {
	nickName = parseString();
}

void PersonInfo::parseSex() {
	sex = parseInt();
}

void PersonInfo::parseAge() {
	age = parseInt();
}

string PersonInfo::parseString() {
	string ret;
	int len = parseShortInt();
	//for (int i = location + 2; i < location + 2 + len; i++) {
	//	ret += decodedInfo[i];
	//}
	ret = decodedInfo.substr(location, len);
	location += len;
	Sleep(1);
	return ret;
}

int32_t PersonInfo::parseLongInt() {
	int32_t ret = 0;
	for (int i = location; i < location + 8; i++) {
		ret = (ret << 8) + (decodedInfo[i] & 0xFF);
	}
	location += 8;
	Sleep(1);
	return ret;
}

int PersonInfo::parseInt() {
	int ret = 0;
	for (int i = location; i < location + 4; i++) {
		ret = (ret << 8) + (decodedInfo[i] & 0xFF);
	}
	location += 4;
	Sleep(1);
	return ret;
}

int PersonInfo::parseShortInt() {
	int ret = 0;
	for (int i = location; i < location + 2; i++) {
		ret = (ret << 8) + (decodedInfo[i] & 0xFF);
	}
	location += 2;
	return ret;
}

void PersonInfo::show() {
	for (int i = 1; i <= decodedInfo.size(); i++) {
		printf("%08x ", decodedInfo[i - 1]);
		if (i % 8 == 0) {
			//cout << endl;
		}
	}

	/*/
		PersonInfo info;
		string infoString(Robot::getStrangerInfo(Util::getMasterQQ(), true));
		Robot::sendToMaster(infoString);
		info.parse(infoString);
		char * ret = new char[50];
		sprintf(ret, "qq = %d", info.qq);
		Robot::sendToMaster(ret);
		Robot::sendToMaster(info.nickName);
		sprintf(ret,"sex = %d",info.sex);
		Robot::sendToMaster(ret);
		sprintf(ret,"age = %d",info.age);
		Robot::sendToMaster(ret);
		delete ret;
	//*/
}
