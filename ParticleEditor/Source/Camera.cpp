#include "Camera.h"

const XMVECTOR Camera::Up = { 0, 1, 0, 0 };
const XMVECTOR Camera::Right = { 1, 0, 0, 0 };

Camera::Camera(ID3D11Device *device, float zoom)
	: m_RotX(0), m_RotY(0), m_Zoom(zoom), m_Up(Up), m_Origo({}), m_Forward({}), m_Right({})
{
	m_Buffer = new ConstantBuffer<CameraValues>(device, BufferUsageDynamic, BufferAccessWrite, 1);
}

Camera::~Camera()
{
	delete m_Buffer;
}

void Camera::Update(ID3D11DeviceContext *cxt)
{
	m_Position = XMVector3Transform({ 0, 0, 1 }, XMMatrixRotationRollPitchYaw(m_RotY, m_RotX, 0));
	m_Position *= m_Zoom;
	m_Position += m_Origo;

	m_View = XMMatrixLookAtRH(m_Position, m_Origo, Up);

	CameraValues values = {
		m_View,
		m_Proj,
		m_Position - m_Origo
	};
	auto ptr = m_Buffer->Map(cxt);
	*ptr = values;
	m_Buffer->Unmap(cxt);
}

void Camera::Rotate(float x, float y)
{
	auto dir = XMVECTOR{ x, y, 0, 0 };
	auto angle = XMVectorGetX(XMVector2Length(dir));

	m_RotX += -x * 0.01f;
	m_RotY += -y * 0.01f;
	if (m_RotY > XM_PIDIV2 - 0.1f) m_RotY = XM_PIDIV2 - 0.1f;
	if (m_RotY < -XM_PIDIV2 + 0.1f) m_RotY = -XM_PIDIV2 + 0.1f;
}

void Camera::Pan(float x, float y)
{
	auto dir = XMVECTOR{ x, y, 0, 0 };

	dir *= XMVector3Length(m_Position) * 0.0001f;

	auto right = XMVector3Cross(XMVector3Normalize(m_Origo - m_Position), Up);
	XMVectorSetY(right, 0.f);
	right = XMVector3Normalize(right);

	auto forward = XMVector3Normalize(m_Origo - m_Position);
	auto up = XMVector3Cross(right, forward);

	m_Origo += right * -x * 0.01f;
	m_Origo += up * y * 0.01f;
}

void Camera::Zoom(float amount)
{
	m_Zoom += amount * -0.1f;

	if (m_Zoom == 0.f) m_Zoom = 0.0001f;
}