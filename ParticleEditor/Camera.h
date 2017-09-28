#pragma once

#include <vector>

#include <d3d11.h>
#include <DirectXMath.h>

#include "External/dxerr.h"

#include "Globals.h"

using namespace DirectX;

class Camera
{
public:
	Camera(XMVECTOR pos, XMVECTOR look);
	~Camera();

	void update(float dt, float width, float height);

	__declspec(align(16))
		struct BufferVals {
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX proj;
		XMMATRIX normal;
		XMMATRIX inverse;
	};
	float znear, zfar;
	BufferVals vals;

	XMVECTOR pos, look;
	XMVECTOR target;

	XMVECTOR temp;
	XMVECTOR offset;

	ID3D11Buffer *wvp_buffer;
	ID3D11Buffer *floatwvpBuffer;

	struct testMatrix {
		XMFLOAT4X4 world;
		XMFLOAT4X4 view;
		XMFLOAT4X4 proj;
	} wvpmatrixes;
};

