#include "duplicator.h"
#include "quad.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <iostream>
#include <memory>

/// get IDXGIOutput1 for IDXGIOutputDuplication
static Microsoft::WRL::ComPtr<IDXGIOutput1> GetPrimaryOutput(const Microsoft::WRL::ComPtr<IDXGIDevice> &dxgi)
{
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgi->GetAdapter(&adapter)))
    {
        return nullptr;
    }

    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);
#ifdef _DEBUG
        std::wcout << desc.Description << std::endl;
#endif
    }

    // get output
    Microsoft::WRL::ComPtr<IDXGIOutput> output;
    if (FAILED(adapter->EnumOutputs(0, &output)))
    {
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
    if (FAILED(output.As(&output1)))
    {
        return nullptr;
    }

    return output1;
}

class DesktopDuplicatorImpl
{

    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_dupl;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_shared;

public:
    // create dupl and setup desktop size texture for sharing
    HANDLE CreateDuplAndSharedHandle()
    {
        // create d3d11
        D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_11_0,
        };
        D3D_FEATURE_LEVEL level;
        if (FAILED(D3D11CreateDevice(nullptr,
                                     D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                     D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, _countof(levels), D3D11_SDK_VERSION,
                                     &m_device, &level, &m_context)))
        {
            return nullptr;
        }

        // get dxgi from d3d11
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;
        if (FAILED(m_device.As(&dxgi)))
        {
            return nullptr;
        }

        // get output1 from dxgi
        auto output = GetPrimaryOutput(dxgi);
        if (!output)
        {
            return nullptr;
        }

        {
            DXGI_OUTPUT_DESC desc;
            output->GetDesc(&desc);
#ifdef _DEBUG
            std::wcout << desc.DeviceName << std::endl;
#endif
        }
        if (FAILED(output->DuplicateOutput(m_device.Get(), &m_dupl)))
        {
            return nullptr;
        }

        DXGI_OUTDUPL_DESC duplDesc;
        m_dupl->GetDesc(&duplDesc);

        // create shared texture
        D3D11_TEXTURE2D_DESC desc = {0};
        desc.Format = duplDesc.ModeDesc.Format;
        desc.Width = duplDesc.ModeDesc.Width;
        desc.Height = duplDesc.ModeDesc.Height;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &m_shared)))
        {
            return nullptr;
        }

        // create shared handle
        Microsoft::WRL::ComPtr<IDXGIResource> sharedResource;
        if (FAILED(m_shared.As(&sharedResource)))
        {
            return nullptr;
        }

        HANDLE handle;
        if (FAILED(sharedResource->GetSharedHandle(&handle)))
        {
            return nullptr;
        }

        // set overlay
        return handle;
    }

    // return false if error
    bool Duplicate()
    {
        m_dupl->ReleaseFrame();

        DXGI_OUTDUPL_FRAME_INFO info;
        Microsoft::WRL::ComPtr<IDXGIResource> resource;
        auto hr = m_dupl->AcquireNextFrame(INFINITE, &info, &resource);
        switch (hr)
        {
        case S_OK:
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> duplTexture;
            if (FAILED(resource.As(&duplTexture)))
            {
                return false;
            }

// copy duplTexture to shared
#if 0
            m_context->CopyResource(m_shared.Get(), duplTexture.Get());
#else
            if (!Render(m_shared, duplTexture))
            {
                return false;
            }
#endif
            break;
        }

        case DXGI_ERROR_WAIT_TIMEOUT:
            break;

        case DXGI_ERROR_ACCESS_LOST:
            return false;

        case DXGI_ERROR_INVALID_CALL:
            // not released previous frame
            return false;

        default:
            return false;
        }

        return true;
    }

private:
    std::unique_ptr<QuadRenderer> m_renderer;

    bool Render(const Microsoft::WRL::ComPtr<ID3D11Texture2D> &dst, const Microsoft::WRL::ComPtr<ID3D11Texture2D> &src)
    {
        if (!m_renderer)
        {
            auto renderer = std::make_unique<QuadRenderer>();
            if (!renderer->Initialize(m_device, dst))
            {
                return false;
            }
            m_renderer = std::move(renderer);
        }

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        if (FAILED(m_device->CreateShaderResourceView(src.Get(), nullptr, &srv)))
        {
            return false;
        }

        m_renderer->Render(m_context, srv);
        return true;
    }
};

DesktopDuplicator::DesktopDuplicator()
    : m_impl(new DesktopDuplicatorImpl)
{
}

DesktopDuplicator::~DesktopDuplicator()
{
    delete m_impl;
}

// create dupl and setup desktop size texture for sharing
void *DesktopDuplicator::CreateDuplAndSharedHandle()
{
    return m_impl->CreateDuplAndSharedHandle();
}

// return false if error
bool DesktopDuplicator::Duplicate()
{
    return m_impl->Duplicate();
}

void DesktopDuplicator::Sleep(int ms)
{
    ::Sleep(ms);
}
