#pragma once
//˫���ϵ��ʱ�� ��������ӹ�ϵ��ʱ�� ��Ҫ��ȷ��
struct tagRelationResult
{
	tagRelationRequest	Info;
	bool				IsSrcOrDest;
};
class CRoleEx;
class RoleRelationRequest // ��ϵ����
{
public:
	RoleRelationRequest();
	virtual ~RoleRelationRequest();

	bool OnInit(CRoleEx* pRole);

	void OnUpdateByDay();

	void OnLoadRelationRequestResult(DBO_Cmd_LoadRelationRequest* pMsg);
	bool IsLoadDB(){ return m_IsLoadDB; }
	//
	void OnSendNewRequest(BYTE RelationType, DWORD DestUserID, LPTSTR MessageInfo);
	void OnSendNewRequestDBResult(DBO_Cmd_AddRelationRequest* pMsg);
	//
	void OnBeAddNewrequest(tagRelationRequest& pRequest);
	//
	void OnHandleRequest(DWORD ID, bool Result);
	void OnBeHandleRequest(tagRelationRequest& pRequest,bool Result);//�����Ľ��մ���
	//�ͻ��˵�����
	void SendInfoToClient();
	//
	void OnAddRelationResult(DWORD SrcUserID, DWORD DestUserID, BYTE RelationType, bool Result);
	void OnDelRelation(DWORD DestUserID, BYTE RelationType);
	//
	void OnDelDestRelation(tagRelationRequest& pInfo);

	bool GetRelationRequestMessageStates();
private:
	bool CheckRelationRequestIsTimeOut(tagRelationRequest& pInfo);
	void OnDelRelationRequest(DWORD ID);
private:
	CRoleEx*								m_pRole;
	bool									m_IsLoadDB;
	HashMap<DWORD, tagRelationRequest>		m_RequestMap;//ȫ��������б�
	HashMap<DWORD, tagRelationResult>		m_WriteHandleMap;
};