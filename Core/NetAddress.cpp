#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress(SOCKADDR_IN sockAddr) :
	_sockAddr(sockAddr)
{
}

NetAddress::NetAddress(wstring ip, uint16 port)
{
	::memset(&_sockAddr, 0, sizeof(_sockAddr));
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr = Ip2Address(ip.c_str());
	_sockAddr.sin_port = ::htons(port);
}

wstring NetAddress::GetIpAddress()
{
	WCHAR buffer[100];
	//IPv4 또는 IPv6 인터넷 네트워크 주소를 인터넷 표준 형식의 문자열로 변환.
	::InetNtopW(AF_INET, &_sockAddr.sin_addr, buffer, len32(buffer));
	return wstring(buffer);
}

IN_ADDR NetAddress::Ip2Address(const WCHAR* ip)
{
	IN_ADDR address;
	//표준 텍스트 프레젠테이션 형식의 IPv4 또는 IPv6 인터넷 네트워크 주소를 숫자 이진 형식으로 변환.
	::InetPtonW(AF_INET, ip, &address);
	return address;
}
