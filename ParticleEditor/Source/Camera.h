#pragma once

#include <vector>

#include <d3d11.h>
#include <DirectXMath.h>

#include "External/dxerr.h"
#include "External/Helpers.h"

using namespace DirectX;

struct CameraValues {
	XMMATRIX m_View;
	XMMATRIX m_Proj;
	XMVECTOR m_Pos;
};

class Camera
{
public:
	Camera(ID3D11Device *device, float zoom);
	~Camera();

	void SetProjection(XMMATRIX proj) { m_Proj = proj; }
	void Update(ID3D11DeviceContext *cxt);

	XMMATRIX GetProjection() const { return m_Proj; }
	XMMATRIX GetView() const { return m_View; }

	ConstantBuffer<CameraValues> *GetBuffer() const { return m_Buffer; }

	void Rotate(float x, float y);
	void Pan(float x, float y);
	void Zoom(float amount);

private:
	ConstantBuffer<CameraValues> *m_Buffer;

	XMMATRIX m_View;
	XMMATRIX m_Proj;

	XMVECTOR m_Forward;
	XMVECTOR m_Right;
	XMVECTOR m_Up;
	XMVECTOR m_Position;

	float m_RotX;
	float m_RotY;

	XMVECTOR m_DragStart;
	XMVECTOR m_DragCurrent;

	XMVECTOR m_Origo;
	float m_Zoom;

	static const XMVECTOR Up;
	static const XMVECTOR Right;
};
