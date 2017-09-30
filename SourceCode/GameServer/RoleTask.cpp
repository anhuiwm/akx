#include "Stdafx.h"
#include "RoleTask.h"
#include "RoleEx.h"
#include "FishServer.h"

std::vector<BYTE> RoleTaskManager::g_VecEmpty;
RoleTaskManager::RoleTaskManager()
{
	m_IsLoadDB = false;
	m_IsSendClient = false;
	m_pRole = NULL;
	m_TaskVec.clear();
	int256Handle::Clear(m_JoinTaskLog);
}
RoleTaskManager::~RoleTaskManager()
{
	//SaveTask();//先将当前正在进行的任务全部保存一次
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  ~RoleTaskManager() ", m_pRole->GetUserID(), m_TaskVec.size());

	OnDestroyDay();//删除对象
	OnDestroyWeek();
}
//void RoleTaskManager::OnSaveByUpdate()
//{
//	//服务器端没15分钟保存一次
//	//SaveTask();
//}
RoleTaskBase* RoleTaskManager::QueryTask(BYTE TaskID)
{
	//查找
	if (m_TaskVec.empty())
		return NULL;
	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
	for (; Iter != m_TaskVec.end(); ++Iter)
	{
		if (!(*Iter))
			continue;
		if ((*Iter)->GetTaskID() == TaskID)
		{
			return *Iter;
		}
	}
	return NULL;
}
bool RoleTaskManager::IsCanJoinTask(BYTE TaskID)
{
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d TaskID=%d", m_pRole->GetUserID(), m_TaskVec.size(), TaskID);
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//是否可以接取指定的任务
	//0.任务数量的要求
	if (m_TaskVec.size() >= g_FishServer.GetFishConfig().GetTaskConfig().m_MaxJoinTaskSum)
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  m_MaxJoinTaskSum=%d", m_pRole->GetUserID(), m_TaskVec.size(), g_FishServer.GetFishConfig().GetTaskConfig().m_MaxJoinTaskSum);
		return false;
	}
	//判断 任务是否已经接取了  就是当前Vector里面是否有当前的ID的存在
	if (int256Handle::GetBitStates(m_JoinTaskLog, TaskID))
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  res=%d", m_pRole->GetUserID(), m_TaskVec.size(), int256Handle::GetBitStates(m_JoinTaskLog, TaskID));
		return false;
	}
	//1.判断任务是否已经完成了
	if (int256Handle::GetBitStates(m_pRole->GetRoleInfo().TaskStates, TaskID))
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  TaskStates=%d  res=%d", m_pRole->GetUserID(), m_TaskVec.size(), m_pRole->GetRoleInfo().TaskStates, int256Handle::GetBitStates(m_pRole->GetRoleInfo().TaskStates, TaskID));
		return false;//任务已经完成了
	}
	//2.等级符合要求
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return false;
	}
	if (m_pRole->GetRoleInfo().wLevel < Iter->second.JoinLevel)
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  m_pRole->GetRoleInfo().wLevel=%d ", m_pRole->GetUserID(), m_TaskVec.size(), m_pRole->GetRoleInfo().wLevel);
		return false;
	}
	//判断是否为分组禁止
	if (g_FishServer.GetEventManager().IsGlobleStop(TaskID))
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  g_FishServer.GetEventManager().IsGlobleStop(TaskID)=%d ", m_pRole->GetUserID(), m_TaskVec.size(), g_FishServer.GetEventManager().IsGlobleStop(TaskID));
		return false;
	}
	//3.前置任务是否已经完成了
	if (Iter->second.LowerTaskVec.empty())
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d TaskID=%d  ok", m_pRole->GetUserID(), m_TaskVec.size(), TaskID);
		return true;
	}
	else
	{
		vector<BYTE>::iterator IterLower = Iter->second.LowerTaskVec.begin();
		for (; IterLower != Iter->second.LowerTaskVec.end(); ++IterLower)
		{
			if (!int256Handle::GetBitStates(m_pRole->GetRoleInfo().TaskStates, *IterLower))
			{
				return false;
			}
		}
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d TaskID=%d  ok 2", m_pRole->GetUserID(), m_TaskVec.size(), TaskID);
		return true;
	}
}
//void RoleTaskManager::OnDestroy()
//{
//	if (m_TaskVec.size() == 0)
//		return;
//	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
//	for (; Iter != m_TaskVec.end(); ++Iter)
//	{
//		delete (*Iter);
//	}
//	m_TaskVec.clear();
//	int256Handle::Clear(m_JoinTaskLog);
//}

void RoleTaskManager::OnDestroyDay()
{
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d OnDestroyDay", m_pRole->GetUserID(), m_TaskVec.size());
	ClearJoinTaskLogBySign(DAY_TASK);
	ClearJoinTaskLogBySign(DAY_ACTIVE_TASK);
	if (m_TaskVec.size() == 0)
		return;
	//BYTE MaxDayTaskID = g_FishServer.GetFishConfig().GetTaskConfig().m_MaxDayTaskID;
	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
	for (; Iter != m_TaskVec.end(); /*++Iter*/)
	{
		//if ( (*Iter)->GetTaskID() <= MaxDayTaskID)
		if (GetTaskSign((*Iter)->GetTaskID()) == DAY_TASK || GetTaskSign((*Iter)->GetTaskID()) == DAY_ACTIVE_TASK)
		{
			SAFE_DELETE(*Iter);
			Iter = m_TaskVec.erase(Iter);
		}
		else
		{
			Iter++;
		}
		
	}
	//m_TaskVec.clear();
	//int256Handle::Clear(m_JoinTaskLog);
	//int256Handle::ClearBitStatesBeforeIndex(m_JoinTaskLog, MaxDayTaskID);
}

void RoleTaskManager::OnDestroyWeek()
{
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d OnDestroyWeek", m_pRole->GetUserID(), m_TaskVec.size());
	ClearJoinTaskLogBySign(WEEK_TASK);
	ClearJoinTaskLogBySign(WEEK_ACTIVE_TASK);
	if (m_TaskVec.size() == 0)
		return;
	//BYTE MaxDayTaskID = g_FishServer.GetFishConfig().GetTaskConfig().m_MaxDayTaskID;
	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
	for (; Iter != m_TaskVec.end(); /*++Iter*/)
	{
		//if ((*Iter)->GetTaskID() > MaxDayTaskID)
		if( GetTaskSign((*Iter)->GetTaskID()) == WEEK_TASK  || GetTaskSign((*Iter)->GetTaskID()) == WEEK_ACTIVE_TASK)
		{
			SAFE_DELETE(*Iter);
			Iter = m_TaskVec.erase(Iter);
		}
		else
		{
			Iter++;
		}

	}
	//m_TaskVec.clear();
	//int256Handle::Clear(m_JoinTaskLog);
	//int256Handle::ClearBitStatesAfterIndex(m_JoinTaskLog, MaxDayTaskID);
}

void RoleTaskManager::ClearJoinTaskLogBySign(BYTE Sign)
{
	std::vector<BYTE>& vecTasks = GetTasksBySign( Sign );
	for (auto mem : vecTasks)
	{
		int256Handle::SetBitStates(m_JoinTaskLog, mem, false);
	}
}

bool RoleTaskManager::GetTaskMessageStates()//日常任务状态
{
	//获取是否有任务完成状态
	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
	for (; Iter != m_TaskVec.end(); ++Iter)
	{
		if (GetTaskSign((*Iter)->GetTaskID()) == DAY_TASK || GetTaskSign((*Iter)->GetTaskID()) == DAY_ACTIVE_TASK)
		{
			if ((*Iter)->IsEventFinish())
				return true;
		}

	}
	return false;
}

bool RoleTaskManager::GetWeekTaskMessageStates()//周常任务状态
{
	//获取是否有任务完成状态
	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
	for (; Iter != m_TaskVec.end(); ++Iter)
	{
		if (GetTaskSign((*Iter)->GetTaskID()) == WEEK_TASK || GetTaskSign((*Iter)->GetTaskID()) == WEEK_ACTIVE_TASK)
		{
			if ((*Iter)->IsEventFinish())
				return true;
		}

	}
	return false;
}

void RoleTaskManager::OnResetJoinAllTask()
{
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  OnResetJoinAllTask", m_pRole->GetUserID(), m_TaskVec.size());
	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
	for (; Iter != m_TaskVec.end(); ++Iter)
	{
		if (!(*Iter))
			continue;
		(*Iter)->OnJoinTask();
	}
}

std::vector<BYTE>& RoleTaskManager::GetTasksBySign( BYTE sign)
{
	HashMap<BYTE, std::vector<BYTE>>& TaskSignMems = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskSignMems;
	auto Itr = TaskSignMems.find(sign);
	if (Itr != TaskSignMems.end())
	{
		return Itr->second;
	}
	else
	{
		return g_VecEmpty;
	}
}


BYTE  RoleTaskManager::GetTaskSign( BYTE taskID)
{
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(taskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		return MIN_TASK;
	}
	else
	{
		return Iter->second.Sign;
	}
}

//void RoleTaskManager::SaveTask()
//{
//	if (!m_pRole)
//	{
//		ASSERT(false);
//		return;
//	}
//	//保持当前全部的任务
//	if (m_TaskVec.size() == 0)
//		return;
//	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
//	for (; Iter != m_TaskVec.end(); ++Iter)
//	{
//		if (!(*Iter)->IsNeedSave())
//			continue;
//		DBR_Cmd_SaveRoleTask msg;
//		SetMsgInfo(msg,DBR_SaveRoleTask, sizeof(DBR_Cmd_SaveRoleTask));
//		msg.dwUserID = m_pRole->GetUserID();
//		msg.TaskInfo = (*Iter)->GetTaskInfo();
//		g_FishServer.SendNetCmdToDB(&msg);
//	}
//}
void RoleTaskManager::GetAllNeedSaveTask(vector<tagRoleTaskInfo>& pVec)
{
	pVec.clear();
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//保持当前全部的任务
	if (m_TaskVec.size() == 0)
		return;
	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
	for (; Iter != m_TaskVec.end(); ++Iter)
	{
		if (!(*Iter)->IsNeedSave())
			continue;
		pVec.push_back((*Iter)->GetTaskInfo());
		(*Iter)->OnSave();
	}
}
bool RoleTaskManager::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	SendLoadAllTaskInfoToDB();
	return true;
}
void RoleTaskManager::SendLoadAllTaskInfoToDB()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	/*DBR_Cmd_LoadRoleTask msg;
	SetMsgInfo(msg,DBR_LoadRoleTask, sizeof(DBR_Cmd_LoadRoleTask));
	msg.dwUserID = m_pRole->GetUserID();
	g_FishServer.SendNetCmdToDB(&msg);*/
}
void RoleTaskManager::OnLoadAllTaskInfoByDB(DBO_Cmd_LoadRoleTask* pDB)
{
	if (!pDB)
	{
		ASSERT(false);
		return;
	}
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if ((pDB->States & MsgBegin) != 0)
	{
		m_TaskVec.clear();
	}
	
	//将从数据库里读取出来的数据 和 配置文件进行配合 处理
	if (m_pRole->IsOnceDayOnline())
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d pDB->Sum=%d  IsOnceDay  OnLoadAllTaskInfoByDB", m_pRole->GetUserID(), m_TaskVec.size(), pDB->Sum);
		//同一天登陆 
		//需要对任务进行过滤 判断数据库的任务是否需要删除 或者 根据配置文件的 是否需要添加
		for (WORD i = 0; i < pDB->Sum; ++i)
		{
			//UserID TaskID Param 任务的数据库表 3个参数就可以了
			HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(pDB->Array[i].TaskID);

			if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end() || !IsCanJoinTask(pDB->Array[i].TaskID))
			{
				//将任务删除掉
				//cout << "删除任务 \n";
				DBR_Cmd_DelRoleTask msg;
				SetMsgInfo(msg,DBR_DelRoleTask, sizeof(DBR_Cmd_DelRoleTask));
				msg.dwUserID = m_pRole->GetUserID();
				msg.bTaskID = pDB->Array[i].TaskID;
				g_FishServer.SendNetCmdToSaveDB(&msg);
				continue;
			}
			else
			{
				//生成一个对象
				RoleTaskBase* pBase = CreateTaskByEventID(Iter->second.EventInfo.EventID);//根据事件ID 创建一个任务对象
				if (!pBase || !pBase->OnInit(m_pRole, this, &pDB->Array[i]))
				{
					ASSERT(false);
					continue;
				}
				//存入集合里面去
				m_TaskVec.push_back(pBase);
				int256Handle::SetBitStates(m_JoinTaskLog, pDB->Array[i].TaskID, true);
				continue;
			}
		}
	}
	else
	{
		OnDayChange(false);

		if (!m_pRole->IsOnceWeekOnline())//不是同一周的
		{
			//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d   !IsOnceWeekOnline  OnLoadAllTaskInfoByDB", m_pRole->GetUserID(), m_TaskVec.size());

			OnWeekChange(false);
		}
	}
	if ((pDB->States & MsgEnd) != 0)
	{
		OnJoinTaskByConfig(false);//玩家上线 新接取的 我们一次性的保存 
		m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Task);
		m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_WeekTask);

		m_IsLoadDB = true;
		m_pRole->IsLoadFinish();
	}
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  end  OnLoadAllTaskInfoByDB", m_pRole->GetUserID(), m_TaskVec.size());
}
void RoleTaskManager::OnDayChange(bool join)
{
	m_pRole->ClearRoleDayTaskStates();
	m_pRole->ChangeRoleDayTaskActiviness(0, true);
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  start  OnDayChange", m_pRole->GetUserID(), m_TaskVec.size());
	//当天数发送变化了
	OnClearAllDayTask();
	if (join)
	{
		OnJoinTaskByConfig(false);
	}
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  end  OnDayChange", m_pRole->GetUserID(), m_TaskVec.size());
}

void RoleTaskManager::OnWeekChange(bool join)
{
	m_pRole->ClearRoleWeekTaskStates();
	m_pRole->ChangeRoleWeekTaskActiviness(0, true);
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  start  OnWeekChange", m_pRole->GetUserID(), m_TaskVec.size());
	OnClearAllWeekTask();
	if (join)
	{
		OnJoinTaskByConfig(false);
	}
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  end  OnWeekChange", m_pRole->GetUserID(), m_TaskVec.size());
}

void RoleTaskManager::SendAllTaskToClient()
{
	//将当前正在进行的任务发送到客户端去
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d    SendAllTaskToClient", m_pRole->GetUserID(), m_TaskVec.size());
	//客户端向服务器端询问全部的任务信息 
	DWORD PageSize = sizeof(LC_Cmd_GetRoleTaskInfo)+sizeof(tagRoleTaskInfo)*(m_TaskVec.size() - 1);
	LC_Cmd_GetRoleTaskInfo * msg = (LC_Cmd_GetRoleTaskInfo*)malloc(PageSize);
	if (!msg)
	{
		ASSERT(false);
		return;
	}
	msg->SetCmdType(GetMsgType(Main_Task, LC_GetRoleTaskInfo));
	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
	for (int i = 0; Iter != m_TaskVec.end(); ++Iter, ++i)
	{
		msg->Array[i] = (*Iter)->GetTaskInfo();
	}
	std::vector<LC_Cmd_GetRoleTaskInfo*> pVec;
	SqlitMsg(msg, PageSize, m_TaskVec.size(),true, pVec);
	free(msg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetRoleTaskInfo*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			m_pRole->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
	m_IsSendClient = true;
}
void RoleTaskManager::OnRoleLevelChange()
{
	OnJoinTaskByConfig(false);
}
void RoleTaskManager::OnJoinTaskByConfig(bool IsNeedSave)
{
	if (m_pRole->IsRobot())
	{
		return;
	}
	if (m_TaskVec.size() >= g_FishServer.GetFishConfig().GetTaskConfig().m_MaxJoinTaskSum)//任务已满无法接取
		return;
	//尝试接取任务
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.begin();
	for (; Iter != g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end(); ++Iter)
	{
		if (m_TaskVec.size() < g_FishServer.GetFishConfig().GetTaskConfig().m_MaxJoinTaskSum)
			OnJoinTask(Iter->second.TaskID, IsNeedSave);
		else
			break;
	}
	if (!IsNeedSave)
	{
		//获取全部任务里待保存的 我们直接进行保存起来
		if (!m_pRole)
		{
			ASSERT(false);
			return;
		}
		vector<tagRoleTaskInfo> pVec;
		GetAllNeedSaveTask(pVec);
		if (pVec.empty())
			return;
		//创建一个命令将全部的任务保存起来
		DWORD PageSize = sizeof(DBR_Cmd_SaveAllTask)+(pVec.size() - 1)*sizeof(tagRoleTaskInfo);
		DBR_Cmd_SaveAllTask* msg = (DBR_Cmd_SaveAllTask*)malloc(PageSize);
		if (!msg)
		{
			ASSERT(false);
			return;
		}
		msg->SetCmdType(DBR_SaveAllTask);
		msg->dwUserID = m_pRole->GetUserID();
		for (DWORD i = 0; i < pVec.size(); ++i)
		{
			msg->Array[i] = pVec[i];
		}
		std::vector<DBR_Cmd_SaveAllTask*> pVec2;
		SqlitMsg(msg, PageSize, pVec.size(), false, pVec2);
		free(msg);
		if (!pVec2.empty())
		{
			std::vector<DBR_Cmd_SaveAllTask*>::iterator Iter = pVec2.begin();
			for (; Iter != pVec2.end(); ++Iter)
			{
				g_FishServer.SendNetCmdToSaveDB(*Iter);
				free(*Iter);
			}
			pVec.clear();
		}
	}
}
bool RoleTaskManager::OnFinishTask(BYTE TaskID)//领取任务奖励的时候
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//当一个任务完成的时候 我们进行处理
	//1.判断任务是否存在	
	if (!int256Handle::GetBitStates(m_JoinTaskLog, TaskID))
	{
		return false;
	}
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		return false;
	}
	std::vector<RoleTaskBase*>::iterator IterTask = m_TaskVec.begin();
	for (; IterTask != m_TaskVec.end(); ++IterTask)
	{
		if ((*IterTask)->GetTaskID() == TaskID)
		{
			if (!(*IterTask)->IsEventFinish())
			{
				ASSERT(false);
				if (m_IsSendClient)
				{
					LC_Cmd_GetOnceTaskInfo msg;
					SetMsgInfo(msg,GetMsgType(Main_Task, LC_GetOnceTaskInfo), sizeof(LC_Cmd_GetOnceTaskInfo));
					msg.TaskInfo = (*IterTask)->GetTaskInfo();
					m_pRole->SendDataToClient(&msg);
				}
				return false;
			}
			int256Handle::SetBitStates(m_JoinTaskLog, TaskID, false);
			//3.给予任务的奖励
			m_pRole->OnAddRoleRewardByRewardID(Iter->second.RewardID,TEXT("完成任务 获得奖励"));
			m_pRole->ChangeRoleTaskStates(TaskID, true);

			if (Iter->second.Sign == DAY_TASK)
			{
				//m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Task);
				m_pRole->ChangeRoleDayTaskActiviness(Iter->second.Activeness);
			}
			else if (Iter->second.Sign == WEEK_TASK)
			{
				//m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_WeekTask);
				m_pRole->ChangeRoleWeekTaskActiviness(Iter->second.Activeness);
			}

			if (m_IsSendClient)
			{
				LC_Cmd_GetTaskReward msg;
				SetMsgInfo(msg,GetMsgType(Main_Task, LC_GetTaskReward), sizeof(LC_Cmd_GetTaskReward));
				msg.TaskID = TaskID;
				m_pRole->SendDataToClient(&msg);
			}

			DBR_Cmd_DelRoleTask msgDel;
			SetMsgInfo(msgDel,DBR_DelRoleTask, sizeof(DBR_Cmd_DelRoleTask));
			msgDel.dwUserID = m_pRole->GetUserID();
			msgDel.bTaskID = TaskID;
			g_FishServer.SendNetCmdToSaveDB(&msgDel);
				
			delete *IterTask;
			m_TaskVec.erase(IterTask);
			//6.发送命令到客户端去 玩家指定任务已经领取奖励了 将任务删除掉
			/*LC_Cmd_DelTask msg;
			msg.TaskID = TaskID;
			m_pRole->SendDataToClient(Main_Task, LC_DelTask, &msg, sizeof(msg));*/
			//7.因为玩家完成了一个任务 
			if (Iter->second.UpperTaskID == 0)
				OnJoinTaskByConfig(false);//玩家需要重新接取一个任务 补充缺失
			else
			{
				if(!OnJoinTask(Iter->second.UpperTaskID,true))
					OnJoinTaskByConfig(false);//玩家需要重新接取一个任务 补充缺失
			}

			m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Task);
			m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_WeekTask);
			return true;
		}
	}
	return false;
}
bool RoleTaskManager::OnJoinTask(BYTE TaskID,bool IsNeedSave)//接取一个任务的时候
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	if (m_pRole->IsRobot())
	{
		return false;
	}
	//当玩家接取一个新的任务的时候 我们进行处理
	//1.判断任务是否存在
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		return false;
	}
	if (!IsCanJoinTask(TaskID))
		return false;
	//3.任务是否可以接取 前置函数已经判断过了 无须继续判断 生成任务对象
	RoleTaskBase* pTask = CreateTaskByEventID(Iter->second.EventInfo.EventID);
	if (!pTask)
	{
		ASSERT(false);
		return false;
	}
	m_TaskVec.push_back(pTask);
	pTask->OnInit(m_pRole, this, TaskID, IsNeedSave);//初始化任务
	int256Handle::SetBitStates(m_JoinTaskLog, TaskID, true);
	//4.发送命令到客户端去
	if (m_IsSendClient)
	{
		LC_Cmd_JoinTask msg;
		SetMsgInfo(msg,GetMsgType(Main_Task, LC_JoinTask), sizeof(LC_Cmd_JoinTask));
		msg.TaskID = TaskID;
		m_pRole->SendDataToClient(&msg);
	}
	//5.立刻保存到数据库去
	if (IsNeedSave)
	{
		DBR_Cmd_SaveRoleTask msg;
		SetMsgInfo(msg, DBR_SaveRoleTask, sizeof(DBR_Cmd_SaveRoleTask));
		msg.dwUserID = m_pRole->GetUserID();
		msg.TaskInfo = pTask->GetTaskInfo();
		g_FishServer.SendNetCmdToSaveDB(&msg);
	}
	return true;
}
bool RoleTaskManager::OnDelTask(BYTE TaskID)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//删除一个任务
	//1.任务配置是否存在
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		return false;
	}
	//2.任务是否已经接取了
	if (!int256Handle::GetBitStates(m_JoinTaskLog, TaskID))
	{
		return false;
	}
	//3.将任务删除掉 
	DBR_Cmd_DelRoleTask msg;
	SetMsgInfo(msg,DBR_DelRoleTask, sizeof(DBR_Cmd_DelRoleTask));
	msg.dwUserID = m_pRole->GetUserID();
	msg.bTaskID = TaskID;
	g_FishServer.SendNetCmdToSaveDB(&msg);
	//cout << "删除任务 \n";
	//4.将对象删除掉
	int256Handle::SetBitStates(m_JoinTaskLog, TaskID, false);
	std::vector<RoleTaskBase*>::iterator IterTask = m_TaskVec.begin();
	for (; IterTask != m_TaskVec.end(); ++IterTask)
	{
		if ((*IterTask)->GetTaskID() == TaskID)
		{
			delete *IterTask;
			m_TaskVec.erase(IterTask);
			break;
		}
	}
	//5.通知客户端 玩家放弃任务
	if (m_IsSendClient)
	{
		LC_Cmd_DelTask msgDel;
		SetMsgInfo(msgDel,GetMsgType(Main_Task, LC_DelTask), sizeof(LC_Cmd_DelTask));
		msgDel.TaskID = TaskID;
		m_pRole->SendDataToClient(&msgDel);
	}
	return true;
}
//bool RoleTaskManager::OnClearAllTask()
//{
//	if (!m_pRole)
//	{
//		ASSERT(false);
//		return false;
//	}
//	//将玩家身上接取的全部任务 全部删除掉
//	//1.清空数据集合
//	OnDestroy();
//	//发送命令
//	DBR_Cmd_ClearRoleTask msg;
//	SetMsgInfo(msg,DBR_ClearRoleTask, sizeof(DBR_Cmd_ClearRoleTask));
//	msg.dwUserID = m_pRole->GetUserID();
//	g_FishServer.SendNetCmdToSaveDB(&msg);
//	//2.发送命令到客户端
//	NetCmd pCmd;
//	SetMsgInfo(pCmd,GetMsgType(Main_Task, LC_ClearTask), sizeof(NetCmd));
//	if (m_IsSendClient)
//		m_pRole->SendDataToClient(&pCmd);
//	return true;
//}

bool RoleTaskManager::OnClearAllDayTask()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//将玩家身上接取的全部任务 全部删除掉
	//1.清空数据集合
	OnDestroyDay();
	//发送命令
	DBR_Cmd_ClearRoleDayTask msg;
	SetMsgInfo(msg,DBR_ClearRoleDayTask, sizeof(DBR_Cmd_ClearRoleDayTask));
	msg.dwUserID = m_pRole->GetUserID();
	//msg.byMaxDayTaskID = g_FishServer.GetFishConfig().GetTaskConfig().m_MaxDayTaskID;
	msg.Sign = DAY_TASK;
	msg.AcSign = DAY_ACTIVE_TASK;
	g_FishServer.SendNetCmdToSaveDB(&msg);
	//2.发送命令到客户端
	NetCmd pCmd;
	SetMsgInfo(pCmd,GetMsgType(Main_Task, LC_ClearDayTask), sizeof(NetCmd));
	if (m_IsSendClient)
		m_pRole->SendDataToClient(&pCmd);
	return true;
}

bool RoleTaskManager::OnClearAllWeekTask()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//将玩家身上接取的全部任务 全部删除掉
	//1.清空数据集合
	OnDestroyWeek();
	//发送命令
	DBR_Cmd_ClearRoleWeekTask msg;
	SetMsgInfo(msg, DBR_ClearRoleWeekTask, sizeof(DBR_Cmd_ClearRoleWeekTask));
	msg.dwUserID = m_pRole->GetUserID();
	//msg.byMaxDayTaskID = g_FishServer.GetFishConfig().GetTaskConfig().m_MaxDayTaskID;
	msg.Sign = DAY_TASK;
	msg.AcSign = DAY_ACTIVE_TASK;
	g_FishServer.SendNetCmdToSaveDB(&msg);
	//2.发送命令到客户端
	NetCmd pCmd;
	SetMsgInfo(pCmd, GetMsgType(Main_Task, LC_ClearWeekTask), sizeof(NetCmd));
	if (m_IsSendClient)
		m_pRole->SendDataToClient(&pCmd);
	return true;
}

RoleTaskBase* RoleTaskManager::CreateTaskByEventID(BYTE EventID)
{
	switch (EventID)
	{
	case ET_GetGlobel:
		return new GetGlobelRoleTask();
	case ET_GetMadel:
		return new GetMadelRoleTask();
	case ET_GetCurren:
		return new GetCurrenRoleTask();
	case ET_UpperLevel:
		return new UpperLevelRoleTask();
	case ET_CatchFish:
		return new CatchFishRoleTask();
	case ET_SendGiff:
		return new SendGiffRoleTask();
	case ET_UseSkill:
		return new UseSkillRoleTask();
	case ET_LauncherGlobel:
		return new LauncherGlobelRoleTask();
	case ET_MaxGlobelSum:
		return new MaxGlobelSumRoleTask();
	case ET_ToLevel:
		return new ToLevelRoleTask();
	case ET_MaxCurren:
		return new MaxCurrenRoleTask();
	case ET_Recharge:
		return new RechargeRoleTask();
	case ET_CompleteDayTask:
		return new CompleteDayTaskRoleTask();
	case ET_CompleteWeekTask:
		return new CompleteWeekTaskRoleTask();
	default:
		return NULL;
	}
}
void RoleTaskManager::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (m_TaskVec.empty())
		return;
	std::vector<RoleTaskBase*>::iterator Iter = m_TaskVec.begin();
	for (; Iter != m_TaskVec.end(); ++Iter)
	{
		(*Iter)->OnHandleEvent(EventID, BindParam, Param);
	 }
}

//任务基类
RoleTaskBase::RoleTaskBase()
{
	m_pTaskManager = NULL;
	m_EventIsFinish = false;
	m_IsNeedSave = false;
	//m_TaskConfig = NULL;
	m_TaskInfo.TaskID = 0;
	m_TaskInfo.TaskValue = 0;
	m_TaskInfo.Sign = 1;
}
RoleTaskBase::~RoleTaskBase()
{
}
bool RoleTaskBase::OnInit(CRoleEx* pRole, RoleTaskManager* pManager, tagRoleTaskInfo* pInfo)
{
	if (!pInfo || !pManager || !pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	m_pTaskManager = pManager;
	m_TaskInfo.TaskID = pInfo->TaskID;
	m_TaskInfo.TaskValue = pInfo->TaskValue;
	m_TaskInfo.Sign = pInfo->Sign;
	m_IsNeedSave = false;
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return false;
	}
	//m_TaskConfig = Iter->second;
	m_EventIsFinish = (Iter->second.EventInfo.FinishValue <= m_TaskInfo.TaskValue);
	return true;
}
bool RoleTaskBase::OnInit(CRoleEx* pRole, RoleTaskManager* pManager, BYTE TaskID, bool IsSave)
{
	if (!pManager || !pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	m_pTaskManager = pManager;
	m_TaskInfo.TaskID = TaskID;
	m_TaskInfo.TaskValue = 0;
	m_TaskInfo.Sign = pManager->GetTaskSign(TaskID);
	//m_IsNeedSave = true;//接取任务的时候 立即保存了.
	m_IsNeedSave = !IsSave;
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return false;
	}
	//m_TaskConfig = Iter->second;
	m_EventIsFinish = false;

	//判断当前任务是否需要设置一个初始化的数值 需要特殊处理
	OnJoinTask();
	//m_pRole->UpdateRoleEvent(true, false, false);
	m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Achievement);
	return true;
}
void RoleTaskBase::OnJoinTask()
{

}
void RoleTaskBase::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	//处理逻辑
	if (m_EventIsFinish)
		return;
	/*if (!m_TaskConfig)
	{
		ASSERT(false);
		return;
	}*/
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	if (Iter->second.EventInfo.EventID != EventID)
	{
		return;
	}
	m_TaskInfo.TaskValue += Param;
	m_IsNeedSave = true;
	m_pRole->SetRoleIsNeedSave();//设置玩家需要保存数据了

	if (m_TaskInfo.TaskValue >= Iter->second.EventInfo.FinishValue)
	{
		//任务完成了 
		m_EventIsFinish = true;

		//if (Iter->second.Sign == DAY_TASK)
		//{
		//	m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Task);
		//	m_pRole->ChangeRoleDayTaskActiviness(Iter->second.Activeness);
		//}
		//else if (Iter->second.Sign == WEEK_TASK)
		//{
		//	m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_WeekTask);
		//	m_pRole->ChangeRoleWeekTaskActiviness(Iter->second.Activeness);
		//}

		LC_Cmd_TaskAllEventFinish msg;
		SetMsgInfo(msg,GetMsgType(Main_Task, LC_TaskAllEventFinish), sizeof(LC_Cmd_TaskAllEventFinish));
		msg.TaskID = m_TaskInfo.TaskID;
		m_pRole->SendDataToClient(&msg);
	}
}


//任务具体的对象处理流程
void GetGlobelRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetGlobel || Param == 0)
		return;
	//对数据进行
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void GetMadelRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetMadel || Param == 0)
		return;
	//对数据进行
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void GetCurrenRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetCurren || Param == 0)
		return;
	//对数据进行
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void UpperLevelRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_UpperLevel || Param == 0)
		return;
	//对数据进行
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void CatchFishRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	if (EventID != ET_CatchFish || Param == 0 || (BindParam != Iter->second.EventInfo.BindParam && Iter->second.EventInfo.BindParam != 0xff))
		return;
	//对数据进行
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void SendGiffRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_SendGiff || Param == 0)
		return;
	//对数据进行
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void UseSkillRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	if (EventID != ET_UseSkill || Param == 0 || (BindParam != Iter->second.EventInfo.BindParam && Iter->second.EventInfo.BindParam != 0xff))
		return;
	//对数据进行
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void LauncherGlobelRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	if (EventID != ET_LauncherGlobel || Param == 0 || BindParam < Iter->second.EventInfo.BindParam)
		return;
	//对数据进行
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void MaxGlobelSumRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_MaxGlobelSum || Param == 0 || Param <= m_TaskInfo.TaskValue)
		return;
	//对数据进行
	if (!m_EventIsFinish)
		m_TaskInfo.TaskValue = 0;//先清空 在设置
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}

void ToLevelRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_ToLevel || Param == 0 || Param <= m_TaskInfo.TaskValue)
		return;
	//对数据进行
	if (!m_EventIsFinish)
		m_TaskInfo.TaskValue = 0;//先清空 在设置
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}

void MaxCurrenRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_MaxCurren || Param == 0 || Param <= m_TaskInfo.TaskValue)
		return;
	//对数据进行
	if (!m_EventIsFinish)
		m_TaskInfo.TaskValue = 0;//先清空 在设置
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void RechargeRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_Recharge || Param == 0 || Param <= m_TaskInfo.TaskValue)
		return;
	if (!m_EventIsFinish)
		m_TaskInfo.TaskValue = 0;//先清空 在设置
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}

void CompleteDayTaskRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_CompleteDayTask || Param == 0 /*|| Param <= m_TaskInfo.TaskValue*/)
		return;
	//if (!m_EventIsFinish)
	m_TaskInfo.TaskValue = Param;//清空设置
	Param = 0;
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}

void CompleteWeekTaskRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_CompleteWeekTask || Param == 0 /*|| Param <= m_TaskInfo.TaskValue*/)
		return;
	m_TaskInfo.TaskValue = Param;//清空设置
	Param = 0;
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}


void MaxGlobelSumRoleTask::OnJoinTask()
{
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//设置属性
	if (Iter->second.EventInfo.EventID != ET_MaxGlobelSum)
		return;
	OnHandleEvent(ET_MaxGlobelSum, 0, m_pRole->GetRoleInfo().dwGlobeNum);
}
void ToLevelRoleTask::OnJoinTask()
{
	//设置属性
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//设置属性
	if (Iter->second.EventInfo.EventID != ET_ToLevel)
		return;
	OnHandleEvent(ET_ToLevel, 0,m_pRole->GetRoleInfo().wLevel);
}
void MaxCurrenRoleTask::OnJoinTask()
{
	//设置属性
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//设置属性
	if (Iter->second.EventInfo.EventID != ET_MaxCurren)
		return;
	OnHandleEvent(ET_MaxCurren,0, m_pRole->GetRoleInfo().dwCurrencyNum);
}

void RechargeRoleTask::OnJoinTask()
{
	//设置属性
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//设置属性
	if (Iter->second.EventInfo.EventID != ET_Recharge)
		return;
	//OnHandleEvent(ET_Recharge, 0, m_pRole->GetRoleInfo().TotalRechargeSum);
}

void CompleteDayTaskRoleTask::OnJoinTask()
{
	//设置属性
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//设置属性
	if (Iter->second.EventInfo.EventID != ET_CompleteDayTask)
		return;
	OnHandleEvent(ET_CompleteDayTask, 0, m_pRole->GetRoleInfo().DayTaskActiviness);
}


void CompleteWeekTaskRoleTask::OnJoinTask()
{
	//设置属性
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//设置属性
	if (Iter->second.EventInfo.EventID != ET_CompleteWeekTask)
		return;
	OnHandleEvent(ET_CompleteWeekTask, 0, m_pRole->GetRoleInfo().WeekTaskActiviness);
}