#ifndef QQROBOT_GROUP_MEMBER_INFO_H
#define QQROBOT_GROUP_MEMBER_INFO_H

#include "PersonInfo.h"

class GroupMemberInfo : public PersonInfo 
{
private:
	void parseGroupId();
	void parseGroupCard();
	void parseRegion();
	void parseAddTime();
	void parseLastMessageTime();
	void parseLevel();
	void parsePowerLevel();
	void parseExclusiveTitle();
	void parseExclusiveTitleDeadline();
	void parseHasBadHistory();
	void parseCardChangeble();
public:
	GroupMemberInfo();
	GroupMemberInfo(const string& infoString);
	~GroupMemberInfo();
	int64_t groupId;
	string groupCard;
	string region;
	int addTime;
	int lastMessageTime;
	string level;
	int powerLevel;//1成员2管理3群主
	string exclusiveTitle;
	int exclusiveTitleDeadline;//-1代表不过期
	bool hasBadHistory;
	bool cardChangeble;
	void parse(const char* infoString);
	void parse(const string& infoString);
};

#endif // !QQROBOT_GROUP_MEMBER_INFO_H
