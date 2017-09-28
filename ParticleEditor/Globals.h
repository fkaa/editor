#pragma once

#define EWIDTH 1600
#define EHEIGHT 900

extern IDXGISwapChain *gSwapChain;

extern ID3D11Device *gDevice;
extern ID3D11DeviceContext *gDeviceContext;

extern ID3D11DepthStencilView *gDepthbufferDSV;
extern ID3D11ShaderResourceView *gDepthbufferSRV;
extern ID3D11RenderTargetView *gDepthbufferRTV;
extern ID3D11RenderTargetView *gBackbufferRTV;

extern ID3D11DepthStencilState *gDepthReadWrite;
extern ID3D11DepthStencilState *gDepthRead;

extern HWND wndHandle;