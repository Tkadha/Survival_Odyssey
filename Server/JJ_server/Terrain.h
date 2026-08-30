#pragma once
#include <windows.h>

class Terrain
{
public:
	static std::shared_ptr<Terrain> terrain;

private:
	USHORT* m_pHeightMapPixels;

	int m_nWidth;
	int m_nLength;
	XMFLOAT3 m_xmf3Scale;

	XMFLOAT3 world_scale;

public:
	Terrain(LPCTSTR pFileName, int nWidth, int nLength, XMFLOAT3 xmf3Scale);
	~Terrain(void);

	float GetHeight(float x, float z, bool bReverseQuad = false);
	// z 좌표로 reverse-quad 여부를 자동 판정해서 높이를 구한다.
	// (호출부마다 반복되던 scale/z%2/bReverseQuad 계산을 통합)
	float GetHeightAtQuad(float x, float z);
	XMFLOAT3 GetHeightMapNormal(int x, int z);
	XMFLOAT3 GetScale() { return (m_xmf3Scale); }

	USHORT* GetHeightMapPixels() { return (m_pHeightMapPixels); }
	int GetHeightMapWidth() { return (m_nWidth); }
	int GetHeightMapLength() { return (m_nLength); }
};
