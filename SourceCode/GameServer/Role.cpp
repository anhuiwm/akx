#include "Stdafx.h"
#include "Role.h"
#include "RoleEx.h"
//#include "ServiceUnits.h"
#include"FishLogic\ExtConfig.h"
#include<timeapi.h>
#include "FishLogic\FishScene\FishResData.h"
#include"FishServer.h"
#include "FishLogic\NetCmd.h"

#include "..\CommonFile\DBLogManager.h"
extern void SendLogDB(NetCmd* pCmd);

CRole::CRole()
{
	//m_LauncherType = 0;
	//m_BulletIdx = 0;

	//m_pRoleEx = NULL;
	m_pTableRolemanager = NULL;
	ResetData();
	m_pConfig = g_FishServer.GetTableManager()->GetGameConfig();
	m_IsSendTableMsg = true;
	m_LuckyValue = 0;
	m_dwUpdateRandomtime = 0;
	m_nTableRate = 0;
	m_dwLockEndTime = 0;
	m_nBulletCount = 0;
	m_dwAccelerateTime = 0;
	m_dwLastFireTime = timeGetTime();
	m_dwLastUseSkillTime = timeGetTime();
	//CTraceService::TraceString(TEXT("һ�����Role �Ѿ�����"), TraceLevel_Normal);
}
CRole::~CRole()
{
	m_pRoleEx = NULL;
	//CTraceService::TraceString(TEXT("һ�����Role �Ѿ�����"), TraceLevel_Normal);
}
void CRole::ResetData()
{
	/*m_TableID = 0xFF;
	m_SeatID = 0xFF;*/

	m_pRoleEx = NULL;
	m_ChestManager.Clear();
	m_BulletIdx = 0;
	//m_LauncherType = 0;
	m_nMultipleIndex = 0;
	m_vecLauncherInfo.clear();

	m_nPay = 0;
	m_nRevenue = 0;
	m_fRp = 1.0f;
	m_dwLastFireTime = timeGetTime();
	m_dwLastUseSkillTime = timeGetTime();
	
	ClearComb();
	m_byMinRate = 0;
	m_byMaxRate = 0;

	m_skill.Reset();

	m_LuckyValue = 0;

	m_dwLockEndTime = 0;
	m_nBulletCount = 0;
	//m_pConfig = NULL;

}
//void CRole::OnBeginUpLoadFaceData(WORD Size)
//{ 
//	if (!m_pRoleEx)
//		return;
//	m_pRoleEx->GetRoleFtpManager().OnBeginUpLoadFaceData(Size); 
//}
//void CRole::OnUpLoadFaceData(WORD StarIndex, WORD Length, BYTE* pData)
//{ 
//	if (!m_pRoleEx)
//		return;
//	m_pRoleEx->GetRoleFtpManager().OnUpLoadFaceData(StarIndex, Length, pData); 
//}
bool CRole::OnInit(WORD TableID, BYTE SeatID, CTableRoleManager* pManager)
{
	if (!pManager)
	{
		ASSERT(false);
		return false;
	}
	m_TableID = TableID;
	m_SeatID = SeatID;
	m_pTableRolemanager = pManager;
	m_pRoleEx = NULL;
	m_LuckyValue = 0;
	m_dwLockEndTime = 0;
	m_dwAccelerateTime = 0;
	m_dwLastFireTime = timeGetTime();
	m_dwLastUseSkillTime = timeGetTime();
	m_ChestManager.OnInit(this);
	return true;
}
void CRole::Clear()
{
	//m_pRoleEx = NULL;//��Ӧ�õ���Ҷ������
	ResetData();
}
void CRole::SetUser(CRoleEx* pUser)
{
	if (!pUser)
	{
		ASSERT(false);
		return;
	}
	m_pRoleEx = pUser;
	m_dwLastFireTime = timeGetTime();
	m_dwLastUseSkillTime = timeGetTime();
	for (int i = 0; i < MAX_LAUNCHER_NUM; i++)
	{
		LauncherInfo item = { 0 };
		//item.nMultipleIndex = 0;// pConfig->BulletMultiple(0);
		//item.nEnergy = 0;
		m_vecLauncherInfo.push_back(item);
	}
}
DWORD CRole::GetID()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return 0;
	}
	return m_pRoleEx->GetUserID();
}
LPTSTR CRole::GetNickName()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return NULL;
	}
	return m_pRoleEx->GetRoleInfo().NickName;
}
DWORD CRole::GetFaceID()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return 0;
	}
	return m_pRoleEx->GetFaceID();

}

BYTE CRole::GetVipLevel()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return 0;
	}
	return m_pRoleEx->GetVipLevel();

}

bool CRole::CheckBulletIdx(BYTE& bulletID)
{
	if (!IsActionUser())
		return false;
	bulletID = (m_SeatID << 8) | m_BulletIdx;
	m_BulletIdx++;
	m_BulletIdx %= MAX_BULLET_NUM;
	bulletID = m_BulletIdx;
	return true;
}

byte CRole::GetLauncherType()const
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return 0;
	}
	//return m_LauncherType;
	return m_pRoleEx->GetRoleLauncherManager().GetUsingLauncher();
}

void CRole::SetLauncherType(BYTE launcher)
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return;
	}
	m_pRoleEx->GetRoleLauncherManager().SetUsingLauncher(launcher);
}

byte CRole::GetTempLauncherType()const
{
	return m_TempLanucher;
}

void CRole::SetTempLauncherType(BYTE launcher)
{
	m_TempLanucher = launcher;
}



void CRole::AddLauncher(bool badd)
{
	if (!m_pConfig)
	{
		ASSERT(false);
		return;
	}
	//m_LauncherType = ConvertIntToBYTE((m_LauncherType + (badd ? 1 : -1) + m_pConfig->CannonCount()) % m_pConfig->CannonCount());
	BYTE LauncherType = ConvertIntToBYTE((GetLauncherType() + (badd ? 1 : -1) + m_pConfig->CannonCount()) % m_pConfig->CannonCount());
	SetLauncherType(LauncherType);
}
void CRole::SetLauncher(BYTE LauncherType)
{
	if (LauncherType >= m_vecLauncherInfo.size())
		return;
	//m_LauncherType = LauncherType;
	SetLauncherType(LauncherType);
}
SkillFailedType CRole::UseSkill(SkillType skill)
{
	if (!m_pConfig || !m_pRoleEx || !m_pTableRolemanager)
	{
		ASSERT(false);
		return SFT_INVALID;
	}

	if (!m_pRoleEx->GetRoleLauncherManager().IsCanUserLauncherByID(GetLauncherType()/*m_LauncherType*/))
	{
		return SFT_UNLOCK;
	}
	int nCannonIndex;
	if (m_pConfig->FindCannon(skill, nCannonIndex) && nCannonIndex != GetLauncherType()/*m_LauncherType*/)
	{
		return SFT_BIND;
	}
	if (skill != SKILL_LOCK && skill != SKILL_ACCELERATE && skill != SKILL_CALL && skill != SKILL_FROZEN)
	{
		if (!m_skill.IsCoolDown())//cd time
		{
			return SFT_CD;
		}
	}

	int nGoodId;
	int nGoodConsume;
	//int nPrice;

	m_pConfig->GoodsInfo(skill, m_skill.UsedTimes(static_cast<BYTE>(skill)), nGoodId, nGoodConsume);   // xuda
	//int nBulletRate = m_pConfig->SkillMultiple(skill);//m_pConfig->BulletMultiple(m_nMultipleIndex);
	//������ʹ�ü��� ����������Ʒ
	if (nGoodConsume>0 && !m_pRoleEx->IsRobot() && !m_pRoleEx->GetItemManager().OnDelUserItem(nGoodId, nGoodConsume))//fail 
	{
		//notify client
		return SFT_COUNT;
	}

	if (skill != SKILL_LOCK && skill != SKILL_ACCELERATE && skill != SKILL_CALL && skill != SKILL_FROZEN)
	{
		for (BYTE i = 0; i < m_pTableRolemanager->GetMaxPlayerSum(); i++)
		{
			CRole *pRole = m_pTableRolemanager->GetRoleBySeatID(i);
			if (pRole)
			{
				pRole->OnSkillUsed(m_pConfig->SkillCdTime(static_cast<BYTE>(skill)));
			}
		}
		m_skill.RecordSkill(static_cast<BYTE>(skill));		
	}
	//else//���Լ�����cd
	//{
	//	this->OnSkillUsed(m_pConfig->SkillCdTime(static_cast<BYTE>(skill)));
	//	m_skill.RecordSkill(static_cast<BYTE>(skill));
	//}

	m_dwLastUseSkillTime = timeGetTime();
	GetRoleExInfo()->OnHandleEvent(true, true, true, ET_UseSkill, skill, 1);


	GameTable* pTable = g_FishServer.GetTableManager()->GetTable(GetTableID());
	if (pTable)
		g_DBLogManager.LogToDB(GetRoleExInfo()->GetUserID(), LT_Skill, skill, static_cast<DWORD>((pTable->GetTableTypeID() << 16) + GetRoleExInfo()->GetRoleMonth().GetMonthID()), TEXT("���ʹ�ü���"), SendLogDB);


	if (GetRoleExInfo()->GetRoleMonth().IsInMonthTable())
	{
		GetRoleExInfo()->GetRoleMonth().OnUseMonthSkill(GetRoleExInfo()->GetRoleMonth().GetMonthID());
	}

	return SFT_OK;
}

LaunchFailedType CRole::UseLaser(byte launcherType)
{
	if (!m_skill.IsCoolDown())//cd time
	{
		return LFT_CD;
	}
	ASSERT(/*m_LauncherType*/GetLauncherType()<m_vecLauncherInfo.size());

	if (!m_pConfig)
	{
		ASSERT(false);
		return LFT_INVALID;
	}
	LauncherInfo&  launcher = m_vecLauncherInfo[/*m_LauncherType*/GetLauncherType()];//launcherType==m_LauncherType

	if (launcher.nEnergy < m_pConfig->LaserThreshold(launcherType)*m_pConfig->BulletMultiple(m_nMultipleIndex))//*rate
	{
		//return LFT_ENERGY; //wm todo
	}

	launcher.nEnergy = 0;
	for (BYTE i = 0; i < m_pTableRolemanager->GetMaxPlayerSum(); i++)
	{
		CRole *pRole = m_pTableRolemanager->GetRoleBySeatID(i);
		if (pRole)
		{
			pRole->OnSkillUsed(m_pConfig->LaserCdTime(launcherType));
		}
	}
	return LFT_OK;
}

void CRole::OnCatchFish(CatchType catchType, byte subType,WORD FishType, BYTE byPackageType, int nScore, int nExp)
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return;
	}

	if (byPackageType!=255)
	{
		m_ChestManager.AddChest(byPackageType);
	}
	m_nRevenue += nScore;
	
	if (m_pRoleEx->GetRoleMonth().IsInMonthTable())
		m_pRoleEx->GetRoleMonth().OnChangeRoleMonthPoint(nScore);
	else
	{
		m_pRoleEx->ChangeRoleGlobe(nScore, false, false, false, true);
		//m_pRoleEx->ChangeRoleWeekClobeNum(nScore);
	}
	
	m_pRoleEx->ChangeRoleProduction(nExp);
	if (!m_pRoleEx->GetRoleMonth().IsInMonthTable())//���ڱ����в���Ӿ���
	{
		m_pRoleEx->ChangeRoleExp(nExp, false);
		m_pRoleEx->OnRoleCatchFishByLottery(ConvertWORDToBYTE(FishType), catchType, subType);
	}

	if (!m_pRoleEx->GetRoleMonth().IsInMonthTable())
	{
		m_pRoleEx->OnHandleEvent(true, true, true, ET_CatchFish, (BYTE)FishType, 1);		
		g_FishServer.GetTableManager()->OnChangeTableGlobel(GetTableID(), -nScore,TableRate());
		m_pRoleEx->ChangeRoleTotalFishGlobelSum(nScore);
	}	

	if (!m_pRoleEx->GetRoleMonth().IsInMonthTable())
		m_pRoleEx->GetRoleGameData().OnHandleCatchFish((BYTE)FishType);//��Ҳ���һ�����ʱ��

	if (catchType == CATCH_BULLET)
	{
		//����Ҳ���һ�����ʱ�� �۳����ָ����������
		DWORD DelLuckyValue = nScore;// m_pConfig->FishLuck(FishType)*RoomMultiple();
		DelRoleLuckValue(DelLuckyValue);//�۳�����ֵ
	}	
}
bool CRole::IsFullEnergy()
{
	ASSERT(/*m_LauncherType*/GetLauncherType()<m_vecLauncherInfo.size());
	ASSERT(m_pConfig != null);
	ASSERT(m_pRoleEx != null);
	LauncherInfo&  launcher = m_vecLauncherInfo[/*m_LauncherType*/GetLauncherType()]; //��ʱ�����ε� xuda  ���ڿ���
	if (launcher.nEnergy >= m_pConfig->LaserThreshold(/*m_LauncherType*/GetLauncherType())*m_pConfig->BulletMultiple(m_nMultipleIndex))// *rate 
		return true;
	else
		return false;
}
bool CRole::CheckFire(BYTE &tableType, BYTE byLauncher, DWORD mul)
{
	ASSERT(/*m_LauncherType*/GetLauncherType()<m_vecLauncherInfo.size());
	ASSERT(m_pConfig != null);
	ASSERT(m_pRoleEx != null);

	if (byLauncher != /*m_LauncherType*/GetLauncherType())
	{
		LogInfoToFile("WmBulletErr", "%d������������:%d,:%d", m_pRoleEx->GetRoleInfo().dwUserID, byLauncher, /*m_LauncherType*/GetLauncherType());
		return false;
	}

	if (!m_pRoleEx->GetRoleLauncherManager().IsCanUserLauncherByID(/*m_LauncherType*/GetLauncherType()))
	{
		LogInfoToFile("WmBulletErr", "����δ����:%u, ��̨:%d", m_pRoleEx->GetRoleInfo().dwUserID, /*m_LauncherType*/GetLauncherType());
		return false;
	}

	if (!m_pRoleEx->GetRoleRate().IsCanUseRateIndex(m_nMultipleIndex))//��ǰ���ʲ�����ʹ��
	{
		return false;
	}

	if (IsUseAccelerate())
	{
		if (timeGetTime() - m_dwLastFireTime < m_pConfig->SkillLauncherIntervalTime(SKILL_ACCELERATE) * 1000)//speed up
		{
			LogInfoToFile("WmBulletErr", "���ڷ�����:%u, ��̨:%d", m_pRoleEx->GetRoleInfo().dwUserID, /*m_LauncherType*/GetLauncherType());
			return false;
		}
	}
	else
	{
		if (timeGetTime() - m_dwLastFireTime < m_pConfig->LauncherInterval(GetLauncherType()) * 1000)
		{
			LogInfoToFile("WmBulletErr", "���ڷ�����:%u, ��̨:%d  timeGetTime()=%d  m_dwLastFireTime=%d", m_pRoleEx->GetRoleInfo().dwUserID,GetLauncherType(), timeGetTime(), m_dwLastFireTime);
			return false;
		}
	}
	LauncherInfo&  launcher = m_vecLauncherInfo[/*m_LauncherType*/GetLauncherType()]; //��ʱ�����ε� xuda  ���ڿ���
	if (launcher.nEnergy >= m_pConfig->LaserThreshold(/*m_LauncherType*/GetLauncherType())*m_pConfig->BulletMultiple(m_nMultipleIndex))// *rate 
	{
		LogInfoToFile("WmBulletErr", "��������ֵ����:%u, ��̨:%d", m_pRoleEx->GetRoleInfo().dwUserID, /*m_LauncherType*/GetLauncherType());// wm todo
		return false;
	}

	DWORD nConsume =mul * ( m_pConfig->BulletConsume(/*m_LauncherType*/GetLauncherType())*m_pConfig->BulletMultiple(m_nMultipleIndex) );
	//LogInfoToFile("WmGoldLog.txt", "nConsume=%d mul:%d,  m_nMultipleIndex:%d  m_nMultipleIndex_config:%d", nConsume, mul,  m_nMultipleIndex, m_pConfig->BulletMultiple(m_nMultipleIndex));

	if (m_pRoleEx->GetRoleMonth().IsInMonthTable())
	{
		if (!m_pRoleEx->GetRoleMonth().OnChangeRoleMonthGlobel(nConsume*-1))
		{
			LogInfoToFile("WmBulletErr", "���ڱ����Ҳ���:%u, ��̨:%d", m_pRoleEx->GetRoleInfo().dwUserID, /*m_LauncherType*/GetLauncherType());
			return false;
		}
	}
	else
	{
		if (!m_pRoleEx->ChangeRoleGlobe(nConsume*-1, false))
		{
			LogInfoToFile("WmBulletErr", "���ڽ�Ҳ���:%u, ��̨:%d", m_pRoleEx->GetRoleInfo().dwUserID, /*m_LauncherType*/GetLauncherType());
			return false;
		}
		g_FishServer.GetTableManager()->OnChangeTableGlobel(GetTableID(), nConsume, TableRate());
		m_pRoleEx->ChangeRoleTotalFishGlobelSum(nConsume*-1);
	}
	AddRoleLuckValue(nConsume);
	m_dwLastFireTime = timeGetTime();
	launcher.nEnergy += nConsume; //(m_pConfig->MultipleEffect() == 0 ? m_pConfig->BulletConsume(m_LauncherType): nConsume);
	m_nPay += nConsume;
	m_nBulletCount++;

	GameTable* pTable = g_FishServer.GetTableManager()->GetTable(m_TableID);
	if (pTable)
	{
		if (m_pRoleEx->IsRobot())
		{
			tableType = 100;//�����˲�������
		}
		else
		{
			tableType = pTable->GetTableTypeID();
			//float addStockScore = nConsume*(1.0f - g_FishServer.GetTableManager()->GetGameConfig()->GetStockTax(tableType));
			float taxStockScore = nConsume*(g_FishServer.GetTableManager()->GetGameConfig()->GetStockTax(tableType));//��˰
			float addStockScore = nConsume - taxStockScore;
			g_FishServer.GetTableManager()->GetGameConfig()->AddStaticStockScore(pTable->GetTableTypeID(), addStockScore);
			g_FishServer.GetTableManager()->GetGameConfig()->AddTaxStockScore(pTable->GetTableTypeID(), taxStockScore);
		}
	}
	return true;
}

__int64 CRole::GetGlobel()
{
	if (!m_pRoleEx)
	{
		return 0;
	}
	if (m_pRoleEx->GetRoleMonth().IsInMonthTable())
	{
		return 0;
	}
	return m_pRoleEx->GetGlobel();
}

float CRole::GetRp()
{
	if (!m_pRoleEx)
	{
		return 0.0f;
	}
	if (m_pRoleEx->GetRoleMonth().IsInMonthTable())
	{
		return 0.0f;
	}
	return (float)m_pRoleEx->GetRoleServerInfo().TotalFishGlobelSum / TableRate();
	//return m_fRp;
}
DWORD CRole::GetExp()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return 0;
	}
	return m_pRoleEx->GetExp();
}
WORD  CRole::GetLevel()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return 0;
	}
	if (m_pRoleEx->GetRoleMonth().IsInMonthTable())
	{
		return 0;
	}
	return m_pRoleEx->GetLevel();
}

DWORD CRole::GetRoleLuckyValue()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return 0;
	}
	if (m_pRoleEx->GetRoleMonth().IsInMonthTable())
	{
		return 0;
	}
	return m_LuckyValue / TableRate();
}

DWORD CRole::GetProduction()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return 0;
	}
	if (m_pRoleEx->GetRoleMonth().IsInMonthTable())
	{
		return 0;
	}
	return m_pRoleEx->GetProduction();
}

DWORD CRole::GetGameTime()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return 0;
	}
	if (m_pRoleEx->GetRoleMonth().IsInMonthTable())
	{
		return 0;
	}
	return m_pRoleEx->GetGameTime();
}

void CRole::SetRpEffect(bool badd)
{
	if (badd)
	{
		if (m_nPay != 0)
		{
			m_fRp = (1.0f*m_nRevenue / m_nPay);
		}
		m_nPay = 0;
		m_nRevenue = 0;
	}
	else//reset
	{
		m_fRp = 1.0f;
	}
}
void CRole::AddGameTime(int nSecond)
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return;
	}
	m_pRoleEx->ChangeRoleGameTime(ConvertIntToWORD(nSecond));
}

USHORT CRole::Combo(WORD wCounter)
{
	if (!m_pConfig || !m_pTableRolemanager)
	{
		ASSERT(false);
		return 0;
	}
	for (size_t i = 0; i < m_Combo.m_vecBulletid.size(); i++)
	{
		if (m_Combo.m_vecBulletid[i] == wCounter)
		{
			return 0;
		}
	}
	m_Combo.m_dwEndTime = timeGetTime() + ConvertDoubleToDWORD(m_pConfig->ComboSustainTime() * 1000.0);
	m_Combo.m_vecBulletid.push_back(wCounter);
	if (m_Combo.m_vecBulletid.size() >= 2)
	{
		m_pRoleEx->GetRoleGameData().OnHandleRoleComb();
		byte byHigh = (m_Combo.m_vecBulletid.size()-1) % m_pConfig->ComboBuffCycle() == 0;
		byte byLow = ConvertDWORDToBYTE(m_Combo.m_vecBulletid.size() - 1);
		return (byHigh << 15) | (byLow);		
	}
	return 0;
}
void CRole::BulletGain(USHORT uBulletType, UINT uGain)
{
	if (m_nBulletCount > 0)
	{
		m_nBulletCount--;
	}

	if (!m_pRoleEx)
	{
		ASSERT(false);
		return;
	}	

	if (!m_pRoleEx->GetRoleMonth().IsInMonthTable())
	{
		m_pRoleEx->OnHandleEvent(true, true, true, ET_LauncherGlobel, uGain, 1);//һ�ڻ�ö��ٽ��

		DWORD nConsume = m_pConfig->BulletConsume(0)*m_pConfig->BulletMultiple(m_byMinRate);
		if (m_nBulletCount == 0 && m_pRoleEx->GetRoleInfo().dwGlobeNum<nConsume)//����͵�һ��
		{
			m_pRoleEx->GetaRoleProtect().OnUserNonGlobel();
		}
	}

	

}

void CRole::OnSkillUsed(float nCdtime)
{
	m_skill.EmitSkill(nCdtime);
}
bool CRole::IsNeedLeaveTable()
{
	if (m_pRoleEx->IsRobot())//�����˲���
		return false;
	if (m_pRoleEx&&!m_pRoleEx->GetRoleMonth().IsInMonthTable() && m_pRoleEx->GetRoleInfo().dwGlobeNum<MinBulletCosume())//�ȼý��в���
	{
		return false;
	}
	DWORD LeaveTableTime = g_FishServer.GetFishConfig().GetSystemConfig().LeaveTableNonLauncherMin * 60000;
	DWORD NowTime = timeGetTime();
	if (NowTime - m_dwLastUseSkillTime >= LeaveTableTime && NowTime - m_dwLastFireTime >= LeaveTableTime)
	{
		//�Ʋ�����²���
		return true;
	}
	else
		return false;
}
void CRole::SetBulletRate(USHORT uPoolRate,BYTE byMin, BYTE byMax)
{
	if (!m_pConfig)
	{
		ASSERT(false);
		return;
	}
	if (byMin > byMax)
	{
		return;
	}
	if (byMin >= m_pConfig->RateCount() || byMax >= m_pConfig->RateCount())
	{
		return;
	}


	m_nMultipleIndex = byMin;
	m_byMinRate = byMin;
	m_byMaxRate = byMax;
	m_nTableRate = uPoolRate;

	if (GetRoleExInfo()->IsRobot())
	{
		if (GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate() < m_nMultipleIndex)
		{
			GetRoleExInfo()->GetRoleRate().SetCanUseMaxRate(m_nMultipleIndex);
		}
	}


	//����������ҵı���Ϊ��ǰ���� �� ��ҿ��Գ��ܵ����ֵ
	if(m_pRoleEx->GetRoleMonth().IsInMonthTable())
		m_nMultipleIndex = max(GetRoleExInfo()->GetRoleRate().GetCanUseMinRate(), m_byMinRate);
	else
		m_nMultipleIndex  = min(GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate(), m_byMaxRate);

	//LogInfoToFile("WmRate.txt", "userid=%d, maxrate=%d  m_nMultipleIndex=%d", GetID(), GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate(), m_nMultipleIndex);
}
void CRole::SetRoomLauncher()
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return;
	}
	//�����䲻Ҫ������m_pRoleEx->GetRoleLauncherManager().ResetUsingLauncher();

	////�л�����ҿ���ʹ�õ���ߵ���
	//for (BYTE i = MAX_LAUNCHER_NUM - 1; i >= 0; --i)
	//{
	//	if (GetRoleExInfo()->GetRoleLauncherManager().IsCanUserLauncherByID(i))
	//	{
	//		m_LauncherType = i;
	//		return;
	//	}
	//}
	//m_LauncherType = 0;
	return;
}
void CRole::ClearComb()
{
	m_Combo.m_ComboBulletID = MAX_BULLET_NUM;
	m_Combo.m_dwEndTime = 0;
	m_Combo.m_vecBulletid.clear();
	m_Combo.m_crit=0;
	
}
bool CRole::IsComboBuff(USHORT BulletID)
{
	if (!m_pConfig)
	{
		ASSERT(false);
		return false;
	}
	if (timeGetTime() >= m_Combo.m_dwEndTime)
	{
		ClearComb();
		return false;
	}
	int nSize = m_Combo.m_vecBulletid.size();
	if (nSize < m_pConfig->ComboBuffCycle())//2������
	{
		return false;
	}
	if ((nSize - 1) % (m_pConfig->ComboBuffCycle()) == 0)//
	{
		//if (m_Combo.m_crit != nSize)
		{
			m_Combo.m_crit = nSize;
			m_Combo.m_ComboBulletID = BulletID;			
		}
		
	}
	return m_Combo.m_ComboBulletID==BulletID;
}
void CRole::Update(DWORD dwTimer)
{
	m_ChestManager.Update(dwTimer);
}

ushort CRole::SkillInfo(SkillType skill)
{
	if (skill < SKILL_MAX)
	{		
		return m_skill.UsedTimes(static_cast<BYTE>(skill));
	}
	return 0;
}
void CRole::OnResetRoleLauncher()
{
	GameTable* pTable = g_FishServer.GetTableManager()->GetTable(m_TableID);
	if (!pTable)
	{
		ASSERT(false);
		return;
	}
	pTable->GetFishDesk()->ResetLauncher(GetID());
}

bool CRole::HaveNormalLauncher(BYTE  byType)
{
	if (!m_pRoleEx || !m_pConfig)
	{
		ASSERT(false);
		return false;
	}
	return m_pRoleEx->GetRoleLauncherManager().IsCanUseLauncherAllTime(byType);
	/*int nGoodsid;
	int nGoodsCount;	
	m_pConfig->GoodsInfo(byType, nGoodsid, nGoodsCount);
	ASSERT(nGoodsid!=0);
	return m_pRoleEx->GetItemManager().GetItemIsAllExists(nGoodsid, 1);*/
}

void CRole::DelRoleLuckValue(DWORD Value)
{
	if (m_LuckyValue < Value)
		m_LuckyValue = 0;
	else
		m_LuckyValue -= Value;	
}

void CRole::AddRoleLuckValue(DWORD Value)
{
	if (m_LuckyValue != 0)//��������
	{
		m_LuckyValue += Value;
	}
	
}

USHORT CRole::TableRate()
{
	return m_nTableRate;
}

void CRole::SetRateIndex(byte idex)
{	
	if (idex<m_byMinRate)
	{
		idex = m_byMinRate;
	}
	if (idex > m_byMaxRate)
	{
		idex = m_byMaxRate;
	}
	m_nMultipleIndex = idex;
	ClearComb();
}

//void CRole::ChangeRateIndex(byte idex)
//{
//	m_nMultipleIndex++;
//	if (m_nMultipleIndex > m_byMaxRate)
//	{
//		m_nMultipleIndex = m_byMinRate;
//	}
//	ClearComb();
//}
void CRole::OnChangeRateToIndex(bool UpOrDown,bool& IsCanUse)
{
	IsCanUse = true;
	BYTE RoleRateMin = max(GetRoleExInfo()->GetRoleRate().GetCanUseMinRate(), m_byMinRate);
	BYTE RoleRateMax = min(GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate(), m_byMaxRate);
	//��÷�Χ�� ���ǽ��д���
	if (UpOrDown)
		m_nMultipleIndex += 1;
	else
		m_nMultipleIndex -= 1;
	if (m_nMultipleIndex > RoleRateMax || m_nMultipleIndex < RoleRateMin)
	{
		if (UpOrDown)
			m_nMultipleIndex = RoleRateMin;
		else
			m_nMultipleIndex = RoleRateMax;
	}
	ClearComb();
	IsCanUse = GetRoleExInfo()->GetRoleRate().IsCanUseRateIndex(m_nMultipleIndex);
}
void CRole::OnChangeRateToIndex(byte RateIndex, bool& IsCanUse)
{
	IsCanUse = true;
	//�����Ҫ����ѡ���� 
	//1.�����ұ��� �� ���� ������
	BYTE RoleRateMin = max(GetRoleExInfo()->GetRoleRate().GetCanUseMinRate(), m_byMinRate);
	BYTE RoleRateMax = min(GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate(), m_byMaxRate);
	//��÷�Χ�� ���ǽ��д���
	if (RateIndex <= RoleRateMax && RateIndex >= RoleRateMin)
	{
		m_nMultipleIndex = RateIndex;
		ClearComb();
	}
	IsCanUse = GetRoleExInfo()->GetRoleRate().IsCanUseRateIndex(m_nMultipleIndex);
}

float CRole::RechargeRate()
{
	if (m_pRoleEx != nullptr && m_pConfig != nullptr)
	{
		if (m_pRoleEx->GetLastRechargeRatioTime() != 0 && m_pRoleEx->GetLastRechargeRatioTime() > time(0))
		{
			return m_pConfig->RechargeRate(m_pRoleEx->GetRechargeRatioSum());
		}
	}
	return 0.0f;
}

float CRole::RandTimeRate()
{
	if (timeGetTime() > m_dwUpdateRandomtime && m_pConfig)
	{
		int TimeSec = 0;
		m_RandomByTime = m_pConfig->RandomTimeRate(TimeSec);
		m_dwUpdateRandomtime = timeGetTime()+TimeSec*1000;
	}
	return m_RandomByTime;
}
//float CRole::RandTimeRate(float fmin, float fmax, BYTE byFishType)
//{
//	if (m_vecRandomByTime.size() == 0)
//	{
//		m_vecRandomByTime.resize(m_pConfig->FishCount());
//	}
//	if (timeGetTime() - m_dwUpdateRandomtime > (DWORD)m_pConfig->RandomCatchCycle())
//	{
//		m_dwUpdateRandomtime = timeGetTime();
//	
//		int nLef = (int)(fmin * 100);
//		int Right = (int)(fmax * 100);
//		if (nLef <Right)
//		{
//			for (size_t i = 0; i < m_vecRandomByTime.size(); i++)
//			{
//				m_vecRandomByTime[i] = nLef + rand() % (Right - nLef);
//			}	
//		}
//	}
//	return (float)m_vecRandomByTime[byFishType]/100.f;
//}
void  CRole::SetLockEndTime()
{ 
	DWORD NowTime = timeGetTime();
	m_dwLockEndTime = (NowTime + m_pConfig->GetSkillCDTime(SkillType::SKILL_LOCK) * 1000);
}

void CRole::SetAccelerateTime()
{
	DWORD NowTime = timeGetTime();
	m_dwAccelerateTime = (NowTime + m_pConfig->GetSkillCDTime(SkillType::SKILL_ACCELERATE) * 1000);
}

bool CRole::IsCanLock()//�ܷ�ʹ����������
{ 
	//return false;//wm todo
	DWORD NowTime = timeGetTime();
	if (m_dwLockEndTime == 0 || m_dwLockEndTime < NowTime)
	{ 
		//m_dwLockEndTime = 0; 
		return true;
	} 
	else
	{
		return false; 
	} 
}


bool CRole::IsUseLock()//���������ڼ䷵��
{
	//return false;//wm todo
	DWORD NowTime = timeGetTime();
	if (m_dwLockEndTime > NowTime)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CRole::IsCanAccelerate()//�ܷ�ʹ����������
{
	//return false;//wm todo
	DWORD NowTime = timeGetTime();
	if (m_dwAccelerateTime == 0 || m_dwAccelerateTime < NowTime)
	{
		//m_dwLockEndTime = 0;
		return true;
	}
	else
	{
		return false;
	}
}


bool CRole::IsUseAccelerate()//���ټ����ڼ䷵��true
{
	//return false;//wm todo
	DWORD NowTime = timeGetTime();
	if (m_dwAccelerateTime > NowTime)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CRole::AddDropReward(ushort nReward)
{
	if (!m_pRoleEx)
	{
		ASSERT(false);
		return ;
	}
	m_pRoleEx->OnAddRoleRewardByRewardID(nReward, TEXT("���佱��"));
}
ushort CRole::MinBulletCosume()
{
	return 	m_pConfig->BulletConsume(/*m_LauncherType*/GetLauncherType())*m_pConfig->BulletMultiple(m_byMinRate);//��С���ĵ��ӵ�
}
ushort CRole::BulletCosume()
{
	return 	m_pConfig->BulletConsume(/*m_LauncherType*/GetLauncherType())*m_pConfig->BulletMultiple(m_nMultipleIndex);
}

USHORT CRole::CombCount()
{
	return m_Combo.m_vecBulletid.size();
}

void CRole::DelayCombo()
{
	m_Combo.m_dwEndTime += 6500;
}

CTableRoleManager::CTableRoleManager()
{

}
CTableRoleManager::~CTableRoleManager()
{
	Destroy();
}
void CTableRoleManager::OnInit(WORD TableID, BYTE TableMaxPlayerSum)
{
	if (TableMaxPlayerSum == 0)
	{
		ASSERT(false);
		return;
	}
	m_MaxTableUserSum = TableMaxPlayerSum;
	m_TableRoleArray = new CRole[m_MaxTableUserSum];
	if (!m_TableRoleArray)
	{
		ASSERT(false);
		return;
	}
	//CTraceService::TraceString(TEXT("һ������������� �Ѿ�����"), TraceLevel_Normal);
	m_TableID = TableID;
	for (BYTE i = 0; i < m_MaxTableUserSum; ++i)
	{
		m_TableRoleArray[i].OnInit(m_TableID, i,this);
	}
}
void CTableRoleManager::Destroy()
{
	//CTraceService::TraceString(TEXT("һ������������� �Ѿ�����"), TraceLevel_Normal);
	delete[] m_TableRoleArray;
	m_TableRoleArray = NULL;
}
int CTableRoleManager::GetRoleSum()
{
	if (!m_TableRoleArray)
	{
		ASSERT(false);
		return 0;
	}
	int Count = 0;
	for (int i = 0; i < m_MaxTableUserSum; ++i)
	{
		if (m_TableRoleArray[i].IsActionUser())
			++Count;
	}
	return Count;
}
void CTableRoleManager::OnDelAllRole()
{
	if (!m_TableRoleArray)
	{
		ASSERT(false);
		return;
	}
	for (BYTE i = 0; i < m_MaxTableUserSum; ++i)
	{
		if (m_TableRoleArray[i].IsActionUser())
		{
			OnDleRole(i);
		}
	}
}
bool CTableRoleManager::OnDelRole(PlayerID RoleID)
{
	if (!m_TableRoleArray)
	{
		ASSERT(false);
		return false;
	}
	for (BYTE i = 0; i < m_MaxTableUserSum; ++i)
	{
		if (m_TableRoleArray[i].IsActionUser() && m_TableRoleArray[i].GetID() == RoleID)
		{
			OnDleRole(i);
			return true;
		}
	}
	return false;
}
bool CTableRoleManager::OnDleRole(WORD ChairID)
{
	if (!m_TableRoleArray)
	{
		ASSERT(false);
		return false;
	}
	if (ChairID >= m_MaxTableUserSum)
		return false;
	m_TableRoleArray[ChairID].Clear();
	//CTraceService::TraceString(TEXT("һ������Ѿ��뿪������"), TraceLevel_Normal);
	return true;
}
bool CTableRoleManager::OnInitRole(CRoleEx* pEx)
{
	//�����������һλ���
	if (!m_TableRoleArray || !pEx) 
	{
		ASSERT(false);
		return false;
	}
	for (int i = 0; i < m_MaxTableUserSum; ++i)
	{
		if (!m_TableRoleArray[i].IsActionUser())
		{
			m_TableRoleArray[i].SetUser(pEx);
			//CTraceService::TraceString(TEXT("һ������Ѿ���������"), TraceLevel_Normal);
			return true;
		}
	}
	return false;
}
CRole* CTableRoleManager::GetRoleBySeatID(BYTE ChairID)
{
	if (!m_TableRoleArray)
	{
		ASSERT(false);
		return NULL;
	}
	if (ChairID >= m_MaxTableUserSum)
		return NULL;
	else
		return &m_TableRoleArray[ChairID];
}
CRole* CTableRoleManager::GetRoleByUserID(PlayerID RoleID)
{
	if (!m_TableRoleArray)
	{
		ASSERT(false);
		return NULL;
	}
	for (int i = 0; i < m_MaxTableUserSum; ++i)
	{
		if (m_TableRoleArray[i].IsActionUser() && m_TableRoleArray[i].GetID() == RoleID)
			return &m_TableRoleArray[i];
	}
	return NULL;
}
void CTableRoleManager::OnUpdate(DWORD dwTimer)
{
	for (int i = 0; i < m_MaxTableUserSum; ++i)
	{
		if (m_TableRoleArray[i].IsActionUser())
			m_TableRoleArray[i].Update(dwTimer);
	}
}
void CTableRoleManager::SwitchFishTide()
{
	for (int i = 0; i < m_MaxTableUserSum; ++i)
	{
		if (m_TableRoleArray[i].IsActionUser())
			m_TableRoleArray[i].DelayCombo();
	}
}

