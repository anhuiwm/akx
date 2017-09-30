//��Һ�����Ϣ
#pragma once
#include "Stdafx.h"
#include <vector>
//��ϵӦ�����������������ͨѶ����
class CRoleEx;
class RoleManager;
class RoleRelationManager //��ҵĹ�ϵ������
{
public:
	RoleRelationManager();
	virtual ~RoleRelationManager();

	bool	OnInit(CRoleEx* pUser, RoleManager* pManager);
	bool    OnLoadUserRelation();
	void	OnLoadUserRelationResult(DBO_Cmd_LoadUserRelation* pDB);
	void	OnLoadBeUserRelationResult(DBO_Cmd_LoadBeUserRelation* pDB);
	//void	OnLoadBeUserRelationFinish();

	bool	OnAddUserRelation(CL_Cmd_AddUserRelation* pMsg);
	void	OnAddUserRelationResult(DBO_Cmd_AddUserRelation* pDB);

	bool	OnDelUserRelation(CL_Cmd_DelUserRelation* pDB);

	//bool    OnChangeUserRelation(CL_Cmd_ChangeUserRelation* pMsg);

	void   OnAddBeUserRelation(tagBeRoleRelation* pInfo);
	void   OnDelBeUserRelation(DWORD dwDestUserID);
	void   OnChagneBeUserRelation(DWORD dwDestUserID, BYTE bRelationType);

	bool   OnGetUserRelation();
	void   SendRoleRelationToCenter();
	void   OnChangeUserOnline(DWORD dwUserID, bool IsOnline);

	//������������������޸Ĺ�ϵ����
	void   OnChangeRelationLevel(DWORD dwUserID, WORD wLevel);
	void   OnChangeRelationFaceID(DWORD dwUserID, DWORD FaceID);
	void   OnChangeRelationGender(DWORD dwUserID, bool bGender);
	void   OnChangeRelationNickName(DWORD dwUserID, LPTSTR pNickName);
	void   OnChangeRelationTitle(DWORD dwUserID, BYTE TitleID);
	void   OnChangeRelationAchievementPoint(DWORD dwUserID, DWORD dwAchievementPoint);
	void   OnChangeRelationCharmValue(DWORD dwUserID, DWORD pCharm[MAX_CHARM_ITEMSUM]);
	void   OnChangeRelationCLientIP(DWORD dwUserID, DWORD ClientIP);
	void   OnChangeRelationIsShowIpAddress(DWORD dwUserID, bool States);
	void   OnChangeRelationVipLevel(DWORD dwUserID, BYTE VipLevel);
	void   OnChangeRelationWeekGlobal(DWORD dwUserID, __int64 WeekGoldNum);
	void   OnChangeRelationIsInMonthCard(DWORD dwUserID, bool IsInMonthCard);

	bool IsLoadDB(){ return m_IsLoadDB; }

	void ResetClientInfo(){ m_IsLoadToClient = false; }

	tagRoleRelation* QueryRelationInfo(DWORD dwDestUserID);
private:
	bool							m_IsLoadToClient;
	//ָ��
	RoleManager*					m_RoleManager;
	CRoleEx*						m_pUser;

	HashMap<DWORD, tagRoleRelation>	 m_RelationMap;
	HashMap<DWORD, tagBeRoleRelation> m_BeRelationMap;

	//std::vector<tagRoleRelation>	m_RelationVec;//���ȫ���Ĺ�ϵ����
	//HashMap<DWORD, RelationType>   m_BeRelationMap;//������ϵ���б�
	bool							m_IsLoadDB;
};