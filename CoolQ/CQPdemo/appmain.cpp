/*
* CoolQ Demo for VC++ 
* Api Version 9
* Written by Coxxs & Thanks for the help of orzFly
*/

#include "stdafx.h"
#include "string"
#include "cqp.h"
#include "appmain.h" //应用AppID等信息，请正确填写，否则酷Q可能无法加载
#include "DropHandle.h"
#include "GroupMemberInfo.h"
#include "iostream"
#include "Util.h"
#include <list>

using namespace std;

int ac = -1; //AuthCode 调用酷Q的方法时需要用到
bool enabled = false;
DropHandle dropHandle;

/* 
* 返回应用的ApiVer、Appid，打包后将不会调用
*/
CQEVENT(const char*, AppInfo, 0)() {
	return CQAPPINFO;
}


/* 
* 接收应用AuthCode，酷Q读取应用信息后，如果接受该应用，将会调用这个函数并传递AuthCode。
* 不要在本函数处理其他任何代码，以免发生异常情况。如需执行初始化代码请在Startup事件中执行（Type=1001）。
*/
CQEVENT(int32_t, Initialize, 4)(int32_t AuthCode) {
	ac = AuthCode;
	return 0;
}


/*
* Type=1001 酷Q启动
* 无论本应用是否被启用，本函数都会在酷Q启动后执行一次，请在这里执行应用初始化代码。
* 如非必要，不建议在这里加载窗口。（可以添加菜单，让用户手动打开窗口）
*/
CQEVENT(int32_t, __eventStartup, 0)() {

	return 0;
}


/*
* Type=1002 酷Q退出
* 无论本应用是否被启用，本函数都会在酷Q退出前执行一次，请在这里执行插件关闭代码。
* 本函数调用完毕后，酷Q将很快关闭，请不要再通过线程等方式执行其他代码。
*/
CQEVENT(int32_t, __eventExit, 0)() {

	return 0;
}

/*
* Type=1003 应用已被启用
* 当应用被启用后，将收到此事件。
* 如果酷Q载入时应用已被启用，则在_eventStartup(Type=1001,酷Q启动)被调用后，本函数也将被调用一次。
* 如非必要，不建议在这里加载窗口。（可以添加菜单，让用户手动打开窗口）
*/
CQEVENT(int32_t, __eventEnable, 0)() {
	enabled = true;

	//加载掉宝和物品文件
	//string dropFile = CQ_getAppDirectory(ac)
	string dropFile = "";
	char path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);
	dropFile = path;
	dropFile += "\\Boss掉宝资料.txt";

	int ret = dropHandle.InitDropList(dropFile);
	if (ret!= 0)
	{
		CQ_addLog(ac, CQLOG_DEBUG, "", "InitDropList失败");
	}

	string limitFile = path;
	limitFile += "\\三国宣传列表.txt";
	ret = dropHandle.InitLimitList(limitFile);
	if (ret != 0)
	{
		CQ_addLog(ac, CQLOG_DEBUG, "", "InitLimitList失败");
	}

	return 0;
}


/*
* Type=1004 应用将被停用
* 当应用被停用前，将收到此事件。
* 如果酷Q载入时应用已被停用，则本函数*不会*被调用。
* 无论本应用是否被启用，酷Q关闭前本函数都*不会*被调用。
*/
CQEVENT(int32_t, __eventDisable, 0)() {
	enabled = false;
	dropHandle.DestoryDropList();
	dropHandle.DestoryLimitList();
	return 0;
}


/*
* Type=21 私聊消息
* subType 子类型，11/来自好友 1/来自在线状态 2/来自群 3/来自讨论组
*/
CQEVENT(int32_t, __eventPrivateMsg, 24)(int32_t subType, int32_t sendTime, int64_t fromQQ, const char *msg, int32_t font) {

	//如果要回复消息，请调用酷Q方法发送，并且这里 return EVENT_BLOCK - 截断本条消息，不再继续处理  注意：应用优先级设置为"最高"(10000)时，不得使用本返回值
	string resp = "";
	string req = msg;
	do
	{
		size_t position = req.find("#掉宝 ");
		if (position != string::npos && position == 0)
		{
			string name = req.substr(string("#掉宝 ").length());
			position = name.find("【");
			if (position != 0)
			{
				break;
			}
			int tail = name.find("】");
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
			position = req.find("#掉物 ");
			if (position != string::npos && position == 0)
			{
				string item = req.substr(string("#掉物 ").length());
				if (!item.empty() && item.length() >=2 )
				{
					CQ_addLog(ac, CQLOG_DEBUG, "fasd", item.c_str());
					resp = dropHandle.GetNameByItem(item);
				}
			}
			else if (position = req.find("#添加敏感字 "), position != string::npos && position == 0)
			{
				string item = req.substr(string("#添加敏感字 ").length());
				int ret = dropHandle.AddLimitItem(item);
				if (ret == 0)
				{
					item = "添加敏感字：" + item + "成功!";
				}
				else if (ret == -1)
				{
					item = "添加敏感字：" + item + "失败，关键字已经存在!";
				}
				else if (ret == -2)
				{
					item = "添加敏感字：" + item + "失败，敏感字不可少于8位!";
				}
				else
				{
					item = "添加敏感字：" + item + "失败，未知错误!";
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
	//如果不回复消息，交由之后的应用/过滤器处理，这里 return EVENT_IGNORE - 忽略本条消息
	//return EVENT_IGNORE;
}


/*
* Type=2 群消息
*/
CQEVENT(int32_t, __eventGroupMsg, 36)(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, const char *fromAnonymous, const char *msg, int32_t font) {
	string resp = "";
	string req = msg;
	do
	{
		if (req == "登录器" || req == "登陆器")
		{
			string msg = "";
			msg += "下载游戏登录器请在群文件【登录器】目录下载，最新完整登录器,游戏客户端：https://jun384668960.github.io/download.htm";
			CQ_sendGroupMsg(ac, fromGroup, msg.c_str());
		}

		size_t position = req.find("#掉宝 ");
		if (position != string::npos && position == 0)
		{
			string name = req.substr(string("#掉宝 ").length());
			position = name.find("【");
			if (position != 0)
			{
				break;
			}
			int tail = name.find("】");
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
			position = req.find("#掉物 ");
			if (position != string::npos && position == 0)
			{
				string item = req.substr(string("#掉物 ").length());
				if (!item.empty() && item.length() >= 2)
				{
					CQ_addLog(ac, CQLOG_DEBUG, "fasd", item.c_str());
					if (item == "神速丹" || item == "铁腕符" || item == "特效神速丹" || item == "蔘茸酒" || item == "复活卷轴"
						|| item == "心眼符" || item == "刚体符" || item == "老酒" || item == "回城卷轴" || item == "特效神速丹"
						|| item == "华陀散" || item.length() <= 4)
					{
						resp = "因为所查询物品掉落可能引起刷屏，若未能查找关键物品，请私聊查询！";
					}
					else
					{
						resp = dropHandle.GetNameByItem(item);
					}
					
				}
			}
			else//查找是否有宣传敏感字符
			{
				if (dropHandle.IsLimit(req))
				{
					CQ_addLog(ac, CQLOG_DEBUG, "有毒", req.c_str());
					//发送宣传语
					Sleep(1000);
					CQ_sendGroupMsg(ac, fromGroup, "http://v.youku.com/v_show/id_XMTU5Mjc0ODUyNA==.html?sharefrom=iphone \
						情义三国，专注防挂三国，封按键，防辅助，开启游戏自动功能，新版本已经上线，期待您的加入！QQ群：429353324");
					//禁言
					//CQAPI(int32_t) CQ_setGroupBan(int32_t AuthCode, int64_t groupid, int64_t QQID, int64_t duration);
					//CQ_setGroupBan(ac, fromGroup, fromQQ, 3600*24*30);
					//踢人
					//CQAPI(int32_t) CQ_setGroupKick(int32_t AuthCode, int64_t groupid, int64_t QQID, CQBOOL rejectaddrequest);
					CQ_setGroupKick(ac, fromGroup, fromQQ, false);
					
				}
				int64_t loginQ = CQ_getLoginQQ(ac);
				char buffer[32];
				string  loginQQ = _i64toa(loginQ, buffer, 10);
				string loginNick = CQ_getLoginNick(ac);

				if ((position = req.find("#功能"), position != string::npos)
					||(position = req.find(loginQQ), position != string::npos)
					||(position = req.find(loginNick), position != string::npos)
					)
				{
					string msg = "情义三国小秘书，功能：\n1.查询掉宝（发送 #掉宝 【吕布】）\n2.查询掉物（发送 #掉物 修复石）";
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

	//return EVENT_IGNORE; //关于返回值说明, 见“_eventPrivateMsg”函数
}


/*
* Type=4 讨论组消息
*/
CQEVENT(int32_t, __eventDiscussMsg, 32)(int32_t subType, int32_t sendTime, int64_t fromDiscuss, int64_t fromQQ, const char *msg, int32_t font) {

	return EVENT_IGNORE; //关于返回值说明, 见“_eventPrivateMsg”函数
}


/*
* Type=101 群事件-管理员变动
* subType 子类型，1/被取消管理员 2/被设置管理员
*/
CQEVENT(int32_t, __eventSystem_GroupAdmin, 24)(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t beingOperateQQ) {

	return EVENT_IGNORE; //关于返回值说明, 见“_eventPrivateMsg”函数
}


/*
* Type=102 群事件-群成员减少
* subType 子类型，1/群员离开 2/群员被踢 3/自己(即登录号)被踢
* fromQQ 操作者QQ(仅subType为2、3时存在)
* beingOperateQQ 被操作QQ
*/
CQEVENT(int32_t, __eventSystem_GroupMemberDecrease, 32)(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, int64_t beingOperateQQ) {

	return EVENT_IGNORE; //关于返回值说明, 见“_eventPrivateMsg”函数
}


/*
* Type=103 群事件-群成员增加
* subType 子类型，1/管理员已同意 2/管理员邀请
* fromQQ 操作者QQ(即管理员QQ)
* beingOperateQQ 被操作QQ(即加群的QQ)
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
		string msg = "欢迎成员：" + name + " 加入情义大家庭！";
		msg += "\r\n下载游戏登录器请在群文件【登录器】目录下载，最新完整登录器,游戏客户端：https://jun384668960.github.io/download.htm";
		CQ_sendGroupMsg(ac, fromGroup, msg.c_str());
		return EVENT_BLOCK;
	}
	else
	{
		return EVENT_IGNORE; //关于返回值说明, 见“_eventPrivateMsg”函数
	}
	

	//string msg = "" + beingOperateQQ;
	//msg = "欢迎成员：" + msg + " 加入情义大家庭！";
	//msg += "\r\n下载游戏登录器请在群文件【登录器】目录下载，微端和最新完整登录器均可";
	//CQ_sendGroupMsg(ac, fromGroup, msg.c_str());
}


/*
* Type=201 好友事件-好友已添加
*/
CQEVENT(int32_t, __eventFriend_Add, 16)(int32_t subType, int32_t sendTime, int64_t fromQQ) {

	return EVENT_IGNORE; //关于返回值说明, 见“_eventPrivateMsg”函数
}


/*
* Type=301 请求-好友添加
* msg 附言
* responseFlag 反馈标识(处理请求用)
*/
CQEVENT(int32_t, __eventRequest_AddFriend, 24)(int32_t subType, int32_t sendTime, int64_t fromQQ, const char *msg, const char *responseFlag) {

	//CQ_setFriendAddRequest(ac, responseFlag, REQUEST_ALLOW, "");

	return EVENT_IGNORE; //关于返回值说明, 见“_eventPrivateMsg”函数
}


/*
* Type=302 请求-群添加
* subType 子类型，1/他人申请入群 2/自己(即登录号)受邀入群
* msg 附言
* responseFlag 反馈标识(处理请求用)
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
	//	string msg = "欢迎成员：" + name + " 加入情义大家庭！";
	//	msg += "\r\n下载游戏登录器请在群文件【登录器】目录下载，微端和最新完整登录器均可";
	//	CQ_sendGroupMsg(ac, fromGroup, msg.c_str());
	//	return EVENT_BLOCK;
	//}
	//else
	//{
	//	return EVENT_IGNORE; //关于返回值说明, 见“_eventPrivateMsg”函数
	//}

	return EVENT_IGNORE; //关于返回值说明, 见“_eventPrivateMsg”函数
}

/*
* 菜单，可在 .json 文件中设置菜单数目、函数名
* 如果不使用菜单，请在 .json 及此处删除无用菜单
*/
CQEVENT(int32_t, __menuA, 0)() {
	MessageBoxA(NULL, "这是menuA，在这里载入窗口，或者进行其他工作。", "" ,0);
	return 0;
}

CQEVENT(int32_t, __menuB, 0)() {
	MessageBoxA(NULL, "这是menuB，在这里载入窗口，或者进行其他工作。", "" ,0);
	return 0;
}
