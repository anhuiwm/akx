//��ҵ�FTP������
//���ڴ���ͻ��˵�����
#pragma once
#include "Stdafx.h"
class CRoleEx;
class RoleFtpManager
{
public:
	RoleFtpManager();
	virtual ~RoleFtpManager();

	bool OnInit(CRoleEx* pRole);

	void OnBeginUpLoadFaceData(WORD DataLength);
	void OnUpLoadFaceData(WORD StarIndex, WORD Length, BYTE* pData);
	void OnUpLoadFaceDataResult(DWORD Crc,bool Result);//�ϴ�ͷ�����յĽ��
private:
	CRoleEx*		m_pRole;
	BYTE*			m_FaceData;
	WORD			m_DataLength;
};