#pragma  once
#include <wdm.h>
#include<ntstrsafe.h>

/************************************************************************/
/* 函数名： 
/* 函数参数：
	 bIsDelete ：是否卸载（TRUE: 卸载服务；FALSE:安装服务）
	 pServiceName ：服务名
	 pGuid : GUID
/* 函数功能：
   向注册表特定设备类中添加通知服务。
 调用方式，示例：
 GagOperClassUsbReg(FALSE,L"myScanDriver\0",L"{4d36e96b-e325-11ce-bfc1-08002be10318}");
 在{4d36e96b-e325-11ce-bfc1-08002be10318}添加服务myScanDriver。
/************************************************************************/
VOID GagOperClassUsbReg(int bIsDelete,PWCHAR pServiceName,PWCHAR pGuid);
