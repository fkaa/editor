#pragma once

#include <windows.h>
#include <comdef.h>

#include <string>
#include <vector>
#include <ctype.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#include <DirectXMath.h>
#include <imgui.h>

#include "dxerr.h"

using namespace DirectX;

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }


inline float RandomFloat(float lo, float hi)
{
	return ((hi - lo) * ((float)rand() / RAND_MAX)) + lo;
}

ID3DBlob *compile_shader(const wchar_t *filename, const char *function, const char *model, ID3D11Device *gDevice);
ID3D11InputLayout *create_input_layout(const D3D11_INPUT_ELEMENT_DESC *elements, size_t size, void const* bytecode, size_t len, ID3D11Device *gDevice);

XMFLOAT4 normalize_color(int32_t color);

std::wstring ConvertToWString(const std::string & s);

LPSTR WinErrorMsg(int nErrorCode, LPSTR pStr, WORD wLength);

enum BufferAccess {
	BufferAccessNone = 0,
	BufferAccessWrite = D3D11_CPU_ACCESS_WRITE,
	BufferAccessRead = D3D11_CPU_ACCESS_READ,
	BufferAccessWriteRead = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
};

enum BufferUsage {
	BufferUsageDefault = D3D11_USAGE_DEFAULT,
	BufferUsageImmutable = D3D11_USAGE_IMMUTABLE,
	BufferUsageDynamic = D3D11_USAGE_DYNAMIC,
	BufferUsageStaging = D3D11_USAGE_STAGING
};

template<int Bind, typename T>
class Buffer {
public:
	Buffer(ID3D11Device *device, BufferUsage usage, BufferAccess access, size_t size, T* ptr = nullptr)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = Bind;
		desc.ByteWidth = max(16, sizeof(T) * size);
		desc.CPUAccessFlags = access;
		desc.Usage = (D3D11_USAGE)usage;

		if (ptr) {
			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = ptr;

			DXCALL(device->CreateBuffer(&desc, &data, &m_Buffer));
		}
		else {
			DXCALL(device->CreateBuffer(&desc, nullptr, &m_Buffer));
		}
	}

	~Buffer()
	{
		if (m_Buffer)
			m_Buffer->Release();
	}

	operator ID3D11Buffer*() { return m_Buffer; }
	operator ID3D11Buffer**() { return &m_Buffer; }

	T* Map(ID3D11DeviceContext *cxt)
	{
		D3D11_MAPPED_SUBRESOURCE data = {};
		cxt->Map(m_Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &data);

		return static_cast<T*>(data.pData);
	}

	void Unmap(ID3D11DeviceContext *cxt)
	{
		cxt->Unmap(m_Buffer, 0);
	}
private:
	ID3D11Buffer *m_Buffer;
};

template<typename T>
using ConstantBuffer = Buffer<D3D11_BIND_CONSTANT_BUFFER, T>;

template<typename T>
using VertexBuffer = Buffer<D3D11_BIND_VERTEX_BUFFER, T>;

template<typename T>
using IndexBuffer = Buffer<D3D11_BIND_INDEX_BUFFER, T>;

