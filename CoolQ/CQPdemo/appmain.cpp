/*
* CoolQ Demo for VC++ 
* Api Version 9
* Written by Coxxs & Thanks for the help of orzFly
*/

#include "stdafx.h"
#include "string"
#include "cqp.h"
#include "appmain.h" //Ӧ��AppID����Ϣ������ȷ��д�������Q�����޷�����
#include "DropHandle.h"
#include "GroupMemberInfo.h"
#include "iostream"
#include "Util.h"
#include <list>

using namespace std;

int ac = -1; //AuthCode ���ÿ�Q�ķ���ʱ��Ҫ�õ�
bool enabled = false;
DropHandle dropHandle;

/* 
* ����Ӧ�õ�ApiVer��Appid������󽫲������
*/
CQEVENT(const char*, AppInfo, 0)() {
	return CQAPPINFO;
}


/* 
* ����Ӧ��AuthCode����Q��ȡӦ����Ϣ��������ܸ�Ӧ�ã���������������������AuthCode��
* ��Ҫ�ڱ��������������κδ��룬���ⷢ���쳣���������ִ�г�ʼ����������Startup�¼���ִ�У�Type=1001����
*/
CQEVENT(int32_t, Initialize, 4)(int32_t AuthCode) {
	ac = AuthCode;
	return 0;
}


/*
* Type=1001 ��Q����
* ���۱�Ӧ���Ƿ����ã������������ڿ�Q������ִ��һ�Σ���������ִ��Ӧ�ó�ʼ�����롣
* ��Ǳ�Ҫ����������������ش��ڡ���������Ӳ˵������û��ֶ��򿪴��ڣ�
*/
CQEVENT(int32_t, __eventStartup, 0)() {

	return 0;
}


/*
* Type=1002 ��Q�˳�
* ���۱�Ӧ���Ƿ����ã������������ڿ�Q�˳�ǰִ��һ�Σ���������ִ�в���رմ��롣
* ������������Ϻ󣬿�Q���ܿ�رգ��벻Ҫ��ͨ���̵߳ȷ�ʽִ���������롣
*/
CQEVENT(int32_t, __eventExit, 0)() {

	return 0;
}

/*
* Type=1003 Ӧ���ѱ�����
* ��Ӧ�ñ����ú󣬽��յ����¼���
* �����Q����ʱӦ���ѱ����ã�����_eventStartup(Type=1001,��Q����)�����ú󣬱�����Ҳ��������һ�Ρ�
* ��Ǳ�Ҫ����������������ش��ڡ���������Ӳ˵������û��ֶ��򿪴��ڣ�
*/
CQEVENT(int32_t, __eventEnable, 0)() {
	enabled = true;

	//���ص�������Ʒ�ļ�
	//string dropFile = CQ_getAppDirectory(ac)
	string dropFile = "";
	char path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);
	dropFile = path;
	dropFile += "\\Boss��������.txt";

	int ret = dropHandle.InitDropList(dropFile);
	if (ret!= 0)
	{
		CQ_addLog(ac, CQLOG_DEBUG, "", "InitDropListʧ��");
	}

	string limitFile = path;
	limitFile += "\\���������б�.txt";
	ret = dropHandle.InitLimitList(limitFile);
	if (ret != 0)
	{
		CQ_addLog(ac, CQLOG_DEBUG, "", "InitLimitListʧ��");
	}

	return 0;
}


/*
* Type=1004 Ӧ�ý���ͣ��
* ��Ӧ�ñ�ͣ��ǰ�����յ����¼���
* �����Q����ʱӦ���ѱ�ͣ�ã��򱾺���*����*�����á�
* ���۱�Ӧ���Ƿ����ã���Q�ر�ǰ��������*����*�����á�
*/
CQEVENT(int32_t, __eventDisable, 0)() {
	enabled = false;
	dropHandle.DestoryDropList();
	dropHandle.DestoryLimitList();
	return 0;
}


/*
* Type=21 ˽����Ϣ
* subType �����ͣ�11/���Ժ��� 1/��������״̬ 2/����Ⱥ 3/����������
*/
CQEVENT(int32_t, __eventPrivateMsg, 24)(int32_t subType, int32_t sendTime, int64_t fromQQ, const char *msg, int32_t font) {

	//���Ҫ�ظ���Ϣ������ÿ�Q�������ͣ��������� return EVENT_BLOCK - �ضϱ�����Ϣ�����ټ�������  ע�⣺Ӧ�����ȼ�����Ϊ"���"(10000)ʱ������ʹ�ñ�����ֵ
	string resp = "";
	string req = msg;
	do
	{
		size_t position = req.find("#���� ");
		if (position != string::npos && position == 0)
		{
			string name = req.substr(string("#���� ").length());
			position = name.find("��");
			if (position != 0)
			{
				break;
			}
			int tail = name.find("��");
			if (tail == string::npos)
			{
				break;
			}

			//string tt = name.substr(2, name.length() - 4);
			//CQ_addLog(ac, CQLOG_DEBUG, "tt", tt.c_str());

			if (!name.empty())
			{
				CQ_addLog(ac, CQLOG_DEBUG, "fasd", name.c_str());
				resp = dropHandle.GetItemsByName(name);
			}
		}
		else
		{
			position = req.find("#���� ");
			if (position != string::npos && position == 0)
			{
				string item = req.substr(string("#���� ").length());
				if (!item.empty() && item.length() >=2 )
				{
					CQ_addLog(ac, CQLOG_DEBUG, "fasd", item.c_str());
					resp = dropHandle.GetNameByItem(item);
				}
			}
			else if (position = req.find("#��������� "), position != string::npos && position == 0)
			{
				string item = req.substr(string("#��������� ").length());
				int ret = dropHandle.AddLimitItem(item);
				if (ret == 0)
				{
					item = "��������֣�" + item + "�ɹ�!";
				}
				else if (ret == -1)
				{
					item = "��������֣�" + item + "ʧ�ܣ��ؼ����Ѿ�����!";
				}
				else if (ret == -2)
				{
					item = "��������֣�" + item + "ʧ�ܣ������ֲ�������8λ!";
				}
				else
				{
					item = "��������֣�" + item + "ʧ�ܣ�δ֪����!";
				}
				CQ_sendPrivateMsg(ac, fromQQ, item.c_str());
			}
		}
	} while (0);
	

	if (!resp.empty())
	{
		CQ_addLog(ac, CQLOG_DEBUG, "string", resp.c_str());
		/*int max = 800;
		wstring _resp = L"";
		bool result = Util::StringToWString(resp, _resp);
		if (result)
		{
			max = 400;
			for (int i = 0; i < _resp.length(); i += max)
			{
				wstring tmp = _resp.substr(i, max);
				string _tmp = "";
				Util::WStringToString(tmp, _tmp);
				if (!tmp.empty())
				{
					CQ_sendPrivateMsg(ac, fromQQ, _tmp.c_str());
				}
			}
		}
		else
		{
			max = 800;
			for (int i = 0; i < resp.length(); i += max)
			{
				string tmp = resp.substr(i, max);

				if (!tmp.empty())
				{
					CQ_sendPrivateMsg(ac, fromQQ, tmp.c_str());
				}
			}
		}*/
		CQ_sendPrivateMsg(ac, fromQQ, resp.c_str());
		return EVENT_BLOCK;
	}
	else
	{
		return EVENT_IGNORE;
	}
	//������ظ���Ϣ������֮���Ӧ��/�������������� return EVENT_IGNORE - ���Ա�����Ϣ
	//return EVENT_IGNORE;
}


/*
* Type=2 Ⱥ��Ϣ
*/
CQEVENT(int32_t, __eventGroupMsg, 36)(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, const char *fromAnonymous, const char *msg, int32_t font) {
	string resp = "";
	string req = msg;
	do
	{
		if (req == "��¼��" || req == "��½��")
		{
			string msg = "";
			msg += "������Ϸ��¼������Ⱥ�ļ�����¼����Ŀ¼���أ�����������¼��,��Ϸ�ͻ��ˣ�https://jun384668960.github.io/download.htm";
			CQ_sendGroupMsg(ac, fromGroup, msg.c_str());
		}

		size_t position = req.find("#���� ");
		if (position != string::npos && position == 0)
		{
			string name = req.substr(string("#���� ").length());
			position = name.find("��");
			if (position != 0)
			{
				break;
			}
			int tail = name.find("��");
			if (tail == string::npos)
			{
				break;
			}

			//string tt = name.substr(2, name.length() - 4);
			//CQ_addLog(ac, CQLOG_DEBUG, "tt", tt.c_str());

			if (!name.empty())
			{
				CQ_addLog(ac, CQLOG_DEBUG, "fasd", name.c_str());
				resp = dropHandle.GetItemsByName(name);
			}
		}
		else
		{
			position = req.find("#���� ");
			if (position != string::npos && position == 0)
			{
				string item = req.substr(string("#���� ").length());
				if (!item.empty() && item.length() >= 2)
				{
					CQ_addLog(ac, CQLOG_DEBUG, "fasd", item.c_str());
					if (item == "���ٵ�" || item == "�����" || item == "��Ч���ٵ�" || item == "�Q�׾�" || item == "�������"
						|| item == "���۷�" || item == "�����" || item == "�Ͼ�" || item == "�سǾ���" || item == "��Ч���ٵ�"
						|| item == "����ɢ" || item.length() <= 4)
					{
						resp = "��Ϊ����ѯ��Ʒ�����������ˢ������δ�ܲ��ҹؼ���Ʒ����˽�Ĳ�ѯ��";
					}
					else
					{
						resp = dropHandle.GetNameByItem(item);
					}
					
				}
			}
			else//�����Ƿ������������ַ�
			{
				if (dropHandle.IsLimit(req))
				{
					CQ_addLog(ac, CQLOG_DEBUG, "�ж�", req.c_str());
					//����������
					Sleep(1000);
					CQ_sendGroupMsg(ac, fromGroup, "http://v.youku.com/v_show/id_XMTU5Mjc0ODUyNA==.html?sharefrom=iphone \
						����������רע�����������ⰴ������������������Ϸ�Զ����ܣ��°汾�Ѿ����ߣ��ڴ����ļ��룡QQȺ��429353324");
					//����
					//CQAPI(int32_t) CQ_setGroupBan(int32_t AuthCode, int64_t groupid, int64_t QQID, int64_t duration);
					//CQ_setGroupBan(ac, fromGroup, fromQQ, 3600*24*30);
					//����
					//CQAPI(int32_t) CQ_setGroupKick(int32_t AuthCode, int64_t groupid, int64_t QQID, CQBOOL rejectaddrequest);
					CQ_setGroupKick(ac, fromGroup, fromQQ, false);
					
				}
				int64_t loginQ = CQ_getLoginQQ(ac);
				char buffer[32];
				string  loginQQ = _i64toa(loginQ, buffer, 10);
				string loginNick = CQ_getLoginNick(ac);

				if ((position = req.find("#����"), position != string::npos)
					||(position = req.find(loginQQ), position != string::npos)
					||(position = req.find(loginNick), position != string::npos)
					)
				{
					string msg = "��������С���飬���ܣ�\n1.��ѯ���������� #���� ����������\n2.��ѯ������� #���� �޸�ʯ��";
					CQ_sendGroupMsg(ac, fromGroup, msg.c_str());
				}
			}
		}
	} while (0);


	if (!resp.empty())
	{
		CQ_addLog(ac, CQLOG_DEBUG, "string", resp.c_str());
		//int max = 800;
		//for (int i = 0; i < resp.length(); i += max)
		//{
		//	string tmp = resp.substr(i, max);
		//	if (!tmp.empty())
		//	{
		//		CQ_sendGroupMsg(ac, fromGroup, tmp.c_str());
		//	}
		//}
		CQ_sendGroupMsg(ac, fromGroup, resp.c_str());
		return EVENT_BLOCK;
	}
	else
	{
		return EVENT_IGNORE;
	}

	//return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}


/*
* Type=4 ��������Ϣ
*/
CQEVENT(int32_t, __eventDiscussMsg, 32)(int32_t subType, int32_t sendTime, int64_t fromDiscuss, int64_t fromQQ, const char *msg, int32_t font) {

	return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}


/*
* Type=101 Ⱥ�¼�-����Ա�䶯
* subType �����ͣ�1/��ȡ������Ա 2/�����ù���Ա
*/
CQEVENT(int32_t, __eventSystem_GroupAdmin, 24)(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t beingOperateQQ) {

	return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}


/*
* Type=102 Ⱥ�¼�-Ⱥ��Ա����
* subType �����ͣ�1/ȺԱ�뿪 2/ȺԱ���� 3/�Լ�(����¼��)����
* fromQQ ������QQ(��subTypeΪ2��3ʱ����)
* beingOperateQQ ������QQ
*/
CQEVENT(int32_t, __eventSystem_GroupMemberDecrease, 32)(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, int64_t beingOperateQQ) {

	return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}


/*
* Type=103 Ⱥ�¼�-Ⱥ��Ա����
* subType �����ͣ�1/����Ա��ͬ�� 2/����Ա����
* fromQQ ������QQ(������ԱQQ)
* beingOperateQQ ������QQ(����Ⱥ��QQ)
*/
CQEVENT(int32_t, __eventSystem_GroupMemberIncrease, 32)(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, int64_t beingOperateQQ) {

	//CQAPI(int32_t) CQ_setGroupCard(int32_t AuthCode, int64_t groupid, int64_t QQID, const char *newcard);
	//CQAPI(const char *) CQ_getGroupMemberInfoV2(int32_t AuthCode, int64_t groupid, int64_t QQID, CQBOOL nocache);
	//CQAPI(const char *) CQ_getStrangerInfo(int32_t AuthCode, int64_t QQID, CQBOOL nocache);

	//string name = CQ_getStrangerInfo(ac, beingOperateQQ, true);
	string baseStr(CQ_getGroupMemberInfoV2(ac, fromGroup, beingOperateQQ, false));
	GroupMemberInfo info;
	info.parse(baseStr);
	string name = info.nickName;

	if (!name.empty())
	{
		string msg = "��ӭ��Ա��" + name + " ����������ͥ��";
		msg += "\r\n������Ϸ��¼������Ⱥ�ļ�����¼����Ŀ¼���أ�����������¼��,��Ϸ�ͻ��ˣ�https://jun384668960.github.io/download.htm";
		CQ_sendGroupMsg(ac, fromGroup, msg.c_str());
		return EVENT_BLOCK;
	}
	else
	{
		return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
	}
	

	//string msg = "" + beingOperateQQ;
	//msg = "��ӭ��Ա��" + msg + " ����������ͥ��";
	//msg += "\r\n������Ϸ��¼������Ⱥ�ļ�����¼����Ŀ¼���أ�΢�˺�����������¼������";
	//CQ_sendGroupMsg(ac, fromGroup, msg.c_str());
}


/*
* Type=201 �����¼�-���������
*/
CQEVENT(int32_t, __eventFriend_Add, 16)(int32_t subType, int32_t sendTime, int64_t fromQQ) {

	return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}


/*
* Type=301 ����-�������
* msg ����
* responseFlag ������ʶ(����������)
*/
CQEVENT(int32_t, __eventRequest_AddFriend, 24)(int32_t subType, int32_t sendTime, int64_t fromQQ, const char *msg, const char *responseFlag) {

	//CQ_setFriendAddRequest(ac, responseFlag, REQUEST_ALLOW, "");

	return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}


/*
* Type=302 ����-Ⱥ���
* subType �����ͣ�1/����������Ⱥ 2/�Լ�(����¼��)������Ⱥ
* msg ����
* responseFlag ������ʶ(����������)
*/
CQEVENT(int32_t, __eventRequest_AddGroup, 32)(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, const char *msg, const char *responseFlag) {

	//if (subType == 1) {
	//	CQ_setGroupAddRequestV2(ac, responseFlag, REQUEST_GROUPADD, REQUEST_ALLOW, "");
	//} 
	////else if (subType == 2) {
	////	CQ_setGroupAddRequestV2(ac, responseFlag, REQUEST_GROUPINVITE, REQUEST_ALLOW, "");
	////}

	//string baseStr(CQ_getStrangerInfo(ac, fromQQ, false));
	//PersonInfo info;
	//info.parse(baseStr);
	//string name = info.nickName;

	//if (!name.empty())
	//{
	//	string msg = "��ӭ��Ա��" + name + " ����������ͥ��";
	//	msg += "\r\n������Ϸ��¼������Ⱥ�ļ�����¼����Ŀ¼���أ�΢�˺�����������¼������";
	//	CQ_sendGroupMsg(ac, fromGroup, msg.c_str());
	//	return EVENT_BLOCK;
	//}
	//else
	//{
	//	return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
	//}

	return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}

/*
* �˵������� .json �ļ������ò˵���Ŀ��������
* �����ʹ�ò˵������� .json ���˴�ɾ�����ò˵�
*/
CQEVENT(int32_t, __menuA, 0)() {
	MessageBoxA(NULL, "����menuA�����������봰�ڣ����߽�������������", "" ,0);
	return 0;
}

CQEVENT(int32_t, __menuB, 0)() {
	MessageBoxA(NULL, "����menuB�����������봰�ڣ����߽�������������", "" ,0);
	return 0;
}
