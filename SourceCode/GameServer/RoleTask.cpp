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
	//SaveTask();//�Ƚ���ǰ���ڽ��е�����ȫ������һ��
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  ~RoleTaskManager() ", m_pRole->GetUserID(), m_TaskVec.size());

	OnDestroyDay();//ɾ������
	OnDestroyWeek();
}
//void RoleTaskManager::OnSaveByUpdate()
//{
//	//��������û15���ӱ���һ��
//	//SaveTask();
//}
RoleTaskBase* RoleTaskManager::QueryTask(BYTE TaskID)
{
	//����
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
	//�Ƿ���Խ�ȡָ��������
	//0.����������Ҫ��
	if (m_TaskVec.size() >= g_FishServer.GetFishConfig().GetTaskConfig().m_MaxJoinTaskSum)
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  m_MaxJoinTaskSum=%d", m_pRole->GetUserID(), m_TaskVec.size(), g_FishServer.GetFishConfig().GetTaskConfig().m_MaxJoinTaskSum);
		return false;
	}
	//�ж� �����Ƿ��Ѿ���ȡ��  ���ǵ�ǰVector�����Ƿ��е�ǰ��ID�Ĵ���
	if (int256Handle::GetBitStates(m_JoinTaskLog, TaskID))
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  res=%d", m_pRole->GetUserID(), m_TaskVec.size(), int256Handle::GetBitStates(m_JoinTaskLog, TaskID));
		return false;
	}
	//1.�ж������Ƿ��Ѿ������
	if (int256Handle::GetBitStates(m_pRole->GetRoleInfo().TaskStates, TaskID))
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  TaskStates=%d  res=%d", m_pRole->GetUserID(), m_TaskVec.size(), m_pRole->GetRoleInfo().TaskStates, int256Handle::GetBitStates(m_pRole->GetRoleInfo().TaskStates, TaskID));
		return false;//�����Ѿ������
	}
	//2.�ȼ�����Ҫ��
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
	//�ж��Ƿ�Ϊ�����ֹ
	if (g_FishServer.GetEventManager().IsGlobleStop(TaskID))
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d  g_FishServer.GetEventManager().IsGlobleStop(TaskID)=%d ", m_pRole->GetUserID(), m_TaskVec.size(), g_FishServer.GetEventManager().IsGlobleStop(TaskID));
		return false;
	}
	//3.ǰ�������Ƿ��Ѿ������
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

bool RoleTaskManager::GetTaskMessageStates()//�ճ�����״̬
{
	//��ȡ�Ƿ����������״̬
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

bool RoleTaskManager::GetWeekTaskMessageStates()//�ܳ�����״̬
{
	//��ȡ�Ƿ����������״̬
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
//	//���ֵ�ǰȫ��������
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
	//���ֵ�ǰȫ��������
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
	
	//�������ݿ����ȡ���������� �� �����ļ�������� ����
	if (m_pRole->IsOnceDayOnline())
	{
		//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d pDB->Sum=%d  IsOnceDay  OnLoadAllTaskInfoByDB", m_pRole->GetUserID(), m_TaskVec.size(), pDB->Sum);
		//ͬһ���½ 
		//��Ҫ��������й��� �ж����ݿ�������Ƿ���Ҫɾ�� ���� ���������ļ��� �Ƿ���Ҫ���
		for (WORD i = 0; i < pDB->Sum; ++i)
		{
			//UserID TaskID Param ��������ݿ�� 3�������Ϳ�����
			HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(pDB->Array[i].TaskID);

			if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end() || !IsCanJoinTask(pDB->Array[i].TaskID))
			{
				//������ɾ����
				//cout << "ɾ������ \n";
				DBR_Cmd_DelRoleTask msg;
				SetMsgInfo(msg,DBR_DelRoleTask, sizeof(DBR_Cmd_DelRoleTask));
				msg.dwUserID = m_pRole->GetUserID();
				msg.bTaskID = pDB->Array[i].TaskID;
				g_FishServer.SendNetCmdToSaveDB(&msg);
				continue;
			}
			else
			{
				//����һ������
				RoleTaskBase* pBase = CreateTaskByEventID(Iter->second.EventInfo.EventID);//�����¼�ID ����һ���������
				if (!pBase || !pBase->OnInit(m_pRole, this, &pDB->Array[i]))
				{
					ASSERT(false);
					continue;
				}
				//���뼯������ȥ
				m_TaskVec.push_back(pBase);
				int256Handle::SetBitStates(m_JoinTaskLog, pDB->Array[i].TaskID, true);
				continue;
			}
		}
	}
	else
	{
		OnDayChange(false);

		if (!m_pRole->IsOnceWeekOnline())//����ͬһ�ܵ�
		{
			//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d   !IsOnceWeekOnline  OnLoadAllTaskInfoByDB", m_pRole->GetUserID(), m_TaskVec.size());

			OnWeekChange(false);
		}
	}
	if ((pDB->States & MsgEnd) != 0)
	{
		OnJoinTaskByConfig(false);//������� �½�ȡ�� ����һ���Եı��� 
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
	//���������ͱ仯��
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
	//����ǰ���ڽ��е������͵��ͻ���ȥ
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//LogInfoToFile("WmTask.txt", "userID=%d, m_TaskVec.size()=%d    SendAllTaskToClient", m_pRole->GetUserID(), m_TaskVec.size());
	//�ͻ������������ѯ��ȫ����������Ϣ 
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
	if (m_TaskVec.size() >= g_FishServer.GetFishConfig().GetTaskConfig().m_MaxJoinTaskSum)//���������޷���ȡ
		return;
	//���Խ�ȡ����
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
		//��ȡȫ�������������� ����ֱ�ӽ��б�������
		if (!m_pRole)
		{
			ASSERT(false);
			return;
		}
		vector<tagRoleTaskInfo> pVec;
		GetAllNeedSaveTask(pVec);
		if (pVec.empty())
			return;
		//����һ�����ȫ�������񱣴�����
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
bool RoleTaskManager::OnFinishTask(BYTE TaskID)//��ȡ��������ʱ��
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//��һ��������ɵ�ʱ�� ���ǽ��д���
	//1.�ж������Ƿ����	
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
			//3.��������Ľ���
			m_pRole->OnAddRoleRewardByRewardID(Iter->second.RewardID,TEXT("������� ��ý���"));
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
			//6.��������ͻ���ȥ ���ָ�������Ѿ���ȡ������ ������ɾ����
			/*LC_Cmd_DelTask msg;
			msg.TaskID = TaskID;
			m_pRole->SendDataToClient(Main_Task, LC_DelTask, &msg, sizeof(msg));*/
			//7.��Ϊ��������һ������ 
			if (Iter->second.UpperTaskID == 0)
				OnJoinTaskByConfig(false);//�����Ҫ���½�ȡһ������ ����ȱʧ
			else
			{
				if(!OnJoinTask(Iter->second.UpperTaskID,true))
					OnJoinTaskByConfig(false);//�����Ҫ���½�ȡһ������ ����ȱʧ
			}

			m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Task);
			m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_WeekTask);
			return true;
		}
	}
	return false;
}
bool RoleTaskManager::OnJoinTask(BYTE TaskID,bool IsNeedSave)//��ȡһ�������ʱ��
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
	//����ҽ�ȡһ���µ������ʱ�� ���ǽ��д���
	//1.�ж������Ƿ����
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		return false;
	}
	if (!IsCanJoinTask(TaskID))
		return false;
	//3.�����Ƿ���Խ�ȡ ǰ�ú����Ѿ��жϹ��� ��������ж� �����������
	RoleTaskBase* pTask = CreateTaskByEventID(Iter->second.EventInfo.EventID);
	if (!pTask)
	{
		ASSERT(false);
		return false;
	}
	m_TaskVec.push_back(pTask);
	pTask->OnInit(m_pRole, this, TaskID, IsNeedSave);//��ʼ������
	int256Handle::SetBitStates(m_JoinTaskLog, TaskID, true);
	//4.��������ͻ���ȥ
	if (m_IsSendClient)
	{
		LC_Cmd_JoinTask msg;
		SetMsgInfo(msg,GetMsgType(Main_Task, LC_JoinTask), sizeof(LC_Cmd_JoinTask));
		msg.TaskID = TaskID;
		m_pRole->SendDataToClient(&msg);
	}
	//5.���̱��浽���ݿ�ȥ
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
	//ɾ��һ������
	//1.���������Ƿ����
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		return false;
	}
	//2.�����Ƿ��Ѿ���ȡ��
	if (!int256Handle::GetBitStates(m_JoinTaskLog, TaskID))
	{
		return false;
	}
	//3.������ɾ���� 
	DBR_Cmd_DelRoleTask msg;
	SetMsgInfo(msg,DBR_DelRoleTask, sizeof(DBR_Cmd_DelRoleTask));
	msg.dwUserID = m_pRole->GetUserID();
	msg.bTaskID = TaskID;
	g_FishServer.SendNetCmdToSaveDB(&msg);
	//cout << "ɾ������ \n";
	//4.������ɾ����
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
	//5.֪ͨ�ͻ��� ��ҷ�������
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
//	//��������Ͻ�ȡ��ȫ������ ȫ��ɾ����
//	//1.������ݼ���
//	OnDestroy();
//	//��������
//	DBR_Cmd_ClearRoleTask msg;
//	SetMsgInfo(msg,DBR_ClearRoleTask, sizeof(DBR_Cmd_ClearRoleTask));
//	msg.dwUserID = m_pRole->GetUserID();
//	g_FishServer.SendNetCmdToSaveDB(&msg);
//	//2.��������ͻ���
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
	//��������Ͻ�ȡ��ȫ������ ȫ��ɾ����
	//1.������ݼ���
	OnDestroyDay();
	//��������
	DBR_Cmd_ClearRoleDayTask msg;
	SetMsgInfo(msg,DBR_ClearRoleDayTask, sizeof(DBR_Cmd_ClearRoleDayTask));
	msg.dwUserID = m_pRole->GetUserID();
	//msg.byMaxDayTaskID = g_FishServer.GetFishConfig().GetTaskConfig().m_MaxDayTaskID;
	msg.Sign = DAY_TASK;
	msg.AcSign = DAY_ACTIVE_TASK;
	g_FishServer.SendNetCmdToSaveDB(&msg);
	//2.��������ͻ���
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
	//��������Ͻ�ȡ��ȫ������ ȫ��ɾ����
	//1.������ݼ���
	OnDestroyWeek();
	//��������
	DBR_Cmd_ClearRoleWeekTask msg;
	SetMsgInfo(msg, DBR_ClearRoleWeekTask, sizeof(DBR_Cmd_ClearRoleWeekTask));
	msg.dwUserID = m_pRole->GetUserID();
	//msg.byMaxDayTaskID = g_FishServer.GetFishConfig().GetTaskConfig().m_MaxDayTaskID;
	msg.Sign = DAY_TASK;
	msg.AcSign = DAY_ACTIVE_TASK;
	g_FishServer.SendNetCmdToSaveDB(&msg);
	//2.��������ͻ���
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

//�������
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
	//m_IsNeedSave = true;//��ȡ�����ʱ�� ����������.
	m_IsNeedSave = !IsSave;
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return false;
	}
	//m_TaskConfig = Iter->second;
	m_EventIsFinish = false;

	//�жϵ�ǰ�����Ƿ���Ҫ����һ����ʼ������ֵ ��Ҫ���⴦��
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
	//�����߼�
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
	m_pRole->SetRoleIsNeedSave();//���������Ҫ����������

	if (m_TaskInfo.TaskValue >= Iter->second.EventInfo.FinishValue)
	{
		//��������� 
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


//�������Ķ���������
void GetGlobelRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetGlobel || Param == 0)
		return;
	//�����ݽ���
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void GetMadelRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetMadel || Param == 0)
		return;
	//�����ݽ���
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void GetCurrenRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetCurren || Param == 0)
		return;
	//�����ݽ���
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void UpperLevelRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_UpperLevel || Param == 0)
		return;
	//�����ݽ���
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
	//�����ݽ���
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void SendGiffRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_SendGiff || Param == 0)
		return;
	//�����ݽ���
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
	//�����ݽ���
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
	//�����ݽ���
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void MaxGlobelSumRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_MaxGlobelSum || Param == 0 || Param <= m_TaskInfo.TaskValue)
		return;
	//�����ݽ���
	if (!m_EventIsFinish)
		m_TaskInfo.TaskValue = 0;//����� ������
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}

void ToLevelRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_ToLevel || Param == 0 || Param <= m_TaskInfo.TaskValue)
		return;
	//�����ݽ���
	if (!m_EventIsFinish)
		m_TaskInfo.TaskValue = 0;//����� ������
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}

void MaxCurrenRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_MaxCurren || Param == 0 || Param <= m_TaskInfo.TaskValue)
		return;
	//�����ݽ���
	if (!m_EventIsFinish)
		m_TaskInfo.TaskValue = 0;//����� ������
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}
void RechargeRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_Recharge || Param == 0 || Param <= m_TaskInfo.TaskValue)
		return;
	if (!m_EventIsFinish)
		m_TaskInfo.TaskValue = 0;//����� ������
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}

void CompleteDayTaskRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_CompleteDayTask || Param == 0 /*|| Param <= m_TaskInfo.TaskValue*/)
		return;
	//if (!m_EventIsFinish)
	m_TaskInfo.TaskValue = Param;//�������
	Param = 0;
	RoleTaskBase::OnHandleEvent(EventID, BindParam, Param);
}

void CompleteWeekTaskRoleTask::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_CompleteWeekTask || Param == 0 /*|| Param <= m_TaskInfo.TaskValue*/)
		return;
	m_TaskInfo.TaskValue = Param;//�������
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
	//��������
	if (Iter->second.EventInfo.EventID != ET_MaxGlobelSum)
		return;
	OnHandleEvent(ET_MaxGlobelSum, 0, m_pRole->GetRoleInfo().dwGlobeNum);
}
void ToLevelRoleTask::OnJoinTask()
{
	//��������
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//��������
	if (Iter->second.EventInfo.EventID != ET_ToLevel)
		return;
	OnHandleEvent(ET_ToLevel, 0,m_pRole->GetRoleInfo().wLevel);
}
void MaxCurrenRoleTask::OnJoinTask()
{
	//��������
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//��������
	if (Iter->second.EventInfo.EventID != ET_MaxCurren)
		return;
	OnHandleEvent(ET_MaxCurren,0, m_pRole->GetRoleInfo().dwCurrencyNum);
}

void RechargeRoleTask::OnJoinTask()
{
	//��������
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//��������
	if (Iter->second.EventInfo.EventID != ET_Recharge)
		return;
	//OnHandleEvent(ET_Recharge, 0, m_pRole->GetRoleInfo().TotalRechargeSum);
}

void CompleteDayTaskRoleTask::OnJoinTask()
{
	//��������
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//��������
	if (Iter->second.EventInfo.EventID != ET_CompleteDayTask)
		return;
	OnHandleEvent(ET_CompleteDayTask, 0, m_pRole->GetRoleInfo().DayTaskActiviness);
}


void CompleteWeekTaskRoleTask::OnJoinTask()
{
	//��������
	HashMap<BYTE, tagTaskConfig>::iterator Iter = g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.find(m_TaskInfo.TaskID);
	if (Iter == g_FishServer.GetFishConfig().GetTaskConfig().m_TaskMap.end())
	{
		ASSERT(false);
		return;
	}
	//��������
	if (Iter->second.EventInfo.EventID != ET_CompleteWeekTask)
		return;
	OnHandleEvent(ET_CompleteWeekTask, 0, m_pRole->GetRoleInfo().WeekTaskActiviness);
}