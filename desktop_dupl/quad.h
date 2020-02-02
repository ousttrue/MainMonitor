#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <stdint.h>

class QuadRenderer
{
    D3D11_VIEWPORT m_vp = {0};
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertices;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indices;
    uint32_t m_indexCount = 0;

public:
    bool Initialize(const Microsoft::WRL::ComPtr<ID3D11Device> &device,
                    const Microsoft::WRL::ComPtr<ID3D11Texture2D> &dst);

    void Render(const Microsoft::WRL::ComPtr<ID3D11DeviceContext> &context,
                const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> &srv);
};
