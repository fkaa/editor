#include "Helpers.h"

#include <stdint.h>
#include <iterator>
#include <fstream>
#include <vector>

#include "dxerr.h"

// Hjälpfunktion för att kompilera shaders, t.ex:
//
//   ID3D11VertexShader *vertexShader = nullptr;
//
//   ID3DBlob *blob = compile_shader(L"Shader.hlsl", "VS", "vs_5_0");
//   DXCALL(gDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &vertexShader));
//
ID3DBlob *compile_shader(const wchar_t *filename, const char *function, const char *model, ID3D11Device *gDevice)
{
	ID3DBlob *blob = nullptr;
	ID3DBlob *error = nullptr;

	DXCALL(
		D3DCompileFromFile(
			filename,
			nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
			function,
			model,
			D3DCOMPILE_DEBUG,
			0,
			&blob,
			&error)
	);

	if (error) {
		OutputDebugStringA((char*)error->GetBufferPointer());
	}

	return blob;
}

// Hjälpfunktion för att skapa input layouts, t.ex:
//
///  ID3D11VertexShader *vertexShader = nullptr;
//   ID3D11InputLayout *inputLayout = nullptr;
//
//   ID3DBlob *blob = CompileShader(L"Shader.hlsl", "VS", "vs_5_0");
///  DXCALL(gDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &vertexShader));
//
///  ...
//
//   D3D11_INPUT_ELEMENT_DESC desc[] = {
//     { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//     { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//	 };
//   create_input_layout(desc, ARRAYSIZE(desc), blob, &inputLayout);
//
ID3D11InputLayout *create_input_layout(const D3D11_INPUT_ELEMENT_DESC *elements, size_t size, void const* bytecode, size_t len, ID3D11Device *gDevice)
{
	ID3D11InputLayout *layout = nullptr;
	DXCALL(gDevice->CreateInputLayout(elements, (UINT)size, bytecode, len, &layout));

	return layout;
}

XMFLOAT4 normalize_color(int32_t color)
{
	return XMFLOAT4(
		((color & 0xff000000) >> 24) / 255.f,
		((color & 0xff0000) >> 16) / 255.f,
		((color & 0xff00) >> 8) / 255.f,
		(color & 0xff) / 255.f
	);
}

std::wstring ConvertToWString(const std::string & s)
{
	const char * cs = s.c_str();
	const size_t wn = std::mbsrtowcs(NULL, &cs, 0, NULL);

	if (wn == size_t(-1))
	{
		return L"";
	}

	std::vector<wchar_t> buf(wn + 1);
	const size_t wn_again = std::mbsrtowcs(buf.data(), &cs, wn + 1, NULL);

	if (wn_again == size_t(-1))
	{
		return L"";
	}

	return std::wstring(buf.data(), wn);
}

LPSTR WinErrorMsg(int nErrorCode, LPSTR pStr, WORD wLength)
{
	try
	{
		LPSTR  szBuffer = pStr;
		int nBufferSize = wLength;

		//
		// prime buffer with error code
		//
		sprintf(szBuffer, "Error code %u", nErrorCode);

		//
		// if we have a message, replace default with msg.
		//
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, nErrorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPSTR)szBuffer,
			nBufferSize,
			NULL);
	}
	catch (...)
	{
	}
	return pStr;
}