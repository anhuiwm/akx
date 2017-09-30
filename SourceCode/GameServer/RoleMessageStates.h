//玩家的 红点状态 由服务器端控制
#pragma once
class CRoleEx;
enum RoleMessageType
{
	//RMT_Mail		= 1,//未读的邮件状态
	//RMT_WeekRank    = 2,//未领取的排行榜奖励
	//RMT_Giff		= 4,//未领取的赠送 
	//RMT_Task		= 8,//任务
	//RMT_Achievement = 16,//成就
	//RMT_Action		= 32,//活动
	//RMT_Check		= 64,//判断玩家今天是否可以签到
	//RMT_Char		= 128,//消息
	//RMT_Relation	= 256,//好友
	//RMT_Online      = 512,//可以在线奖励

	RMT_Mail = 1,//未读的邮件状态
	RMT_WeekRank = 2,//未领取的排行榜奖励（暂时不要）
	RMT_Giff = 4,//未领取的赠送 
	RMT_Task = 8,//日常任务
	RMT_Achievement = 16,//成就
	RMT_Action = 32,//活动
	RMT_Check = 64,//签到
	RMT_Char = 128,//聊天
	RMT_Relation = 256,//好友请求
	RMT_Online = 512,//可以在线奖励
	RMT_WeekTask = 1024,//周常任务
	RMT_Forge = 2048,//锻造  一次性
	//RMT_OnlineReward = 2048,//在线抽奖
};
class RoleMessageStates
{
public:
	RoleMessageStates();
	virtual ~RoleMessageStates();

	void	OnInit(CRoleEx* pRole);

	void	OnChangeRoleMessageStates(RoleMessageType Type,bool IsSendToClient = true, bool Once = false);
private:
	CRoleEx*				m_pRole;
	bool					m_IsInit;
	DWORD					m_StatesValue;
};