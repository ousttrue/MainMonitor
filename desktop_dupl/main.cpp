#include "overlay.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <iostream>

auto CLASS_NAME = L"MinOverlayClass";
auto WINDOW_NAME = L"MinOverlay";

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

class DesktopDuplicator
{
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_dupl;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_shared;

public:
    // create dupl and setup desktop size texture for sharing
    HANDLE CreateDuplAndSharedHandle(const Microsoft::WRL::ComPtr<ID3D11Device> &device,
                                     const Microsoft::WRL::ComPtr<IDXGIOutput1> &output)
    {
        if (FAILED(output->DuplicateOutput(device.Get(), &m_dupl)))
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
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        if (FAILED(device->CreateTexture2D(&desc, nullptr, &m_shared)))
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
    bool Duplicate(const Microsoft::WRL::ComPtr<ID3D11DeviceContext> &context)
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
            context->CopyResource(m_shared.Get(), duplTexture.Get());
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
};

int main(int argc, char **argv)
{
    // openvr overlay
    Overlay overlay;
    if (!overlay.Initialize("desktop_dupl", "desktop dupl"))
    {
        return 1;
    }

    // create d3d11
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL level;
    if (FAILED(D3D11CreateDevice(nullptr,
                                 D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                 D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, _countof(levels), D3D11_SDK_VERSION,
                                 &device, &level, &context)))
    {
        return 2;
    }

    // get dxgi from d3d11
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;
    if (FAILED(device.As(&dxgi)))
    {
        return 2;
    }

    // get output1 from dxgi
    auto output = GetPrimaryOutput(dxgi);
    if (!output)
    {
        return 2;
    }

    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
#ifdef _DEBUG
        std::wcout << desc.DeviceName << std::endl;
#endif
    }

    // create desktop texture
    DesktopDuplicator dupl;
    auto handle = dupl.CreateDuplAndSharedHandle(device, output);
    if (!handle)
    {
        return 3;
    }
    overlay.SetSharedHanle(handle);

    // main loop
    bool isActive;
    while (overlay.Loop(&isActive))
    {
        if (isActive)
        {
            // update shared texture
            dupl.Duplicate(context);
        }
        else
        {
            // wait
            Sleep(100);
        }
    }

    return 0;
}
