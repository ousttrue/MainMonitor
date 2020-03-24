static ComPtr<ID3D12RootSignature> CreateRootSignature(const ComPtr<ID3D12Device> &device)
{
    D3D12_DESCRIPTOR_RANGE descRange = {
        .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        .NumDescriptors = 1,
        .BaseShaderRegister = 0,
        .RegisterSpace = 0,
        .OffsetInDescriptorsFromTableStart = 0,
    };

    D3D12_ROOT_PARAMETER param[] = {
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants = {
                .ShaderRegister = 0,
                .RegisterSpace = 0,
                .Num32BitValues = 16,
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable = {
                .NumDescriptorRanges = 1,
                .pDescriptorRanges = &descRange,
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
        },
    };

    D3D12_STATIC_SAMPLER_DESC staticSampler = {
        .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .MipLODBias = 0.f,
        .MaxAnisotropy = 0,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
        .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
        .MinLOD = 0.f,
        .MaxLOD = 0.f,
        .ShaderRegister = 0,
        .RegisterSpace = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };

    D3D12_ROOT_SIGNATURE_DESC desc = {
        .NumParameters = _countof(param),
        .pParameters = param,
        .NumStaticSamplers = 1,
        .pStaticSamplers = &staticSampler,
        .Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS,
    };

    ComPtr<ID3DBlob> blob = NULL;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, NULL)))
    {
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> pRootSignature;
    if (FAILED(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature))))
    {
        return nullptr;
    }

    return pRootSignature;
}

static ComPtr<ID3D12PipelineState> CreatePipelineState(const ComPtr<ID3D12Device> &device,
                                                       const ComPtr<ID3D12RootSignature> &pRootSignature,
                                                       const char *vertexShader,
                                                       const char *pixelShader)
{
    ComPtr<ID3D10Blob> g_pVertexShaderBlob;
    if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &g_pVertexShaderBlob, NULL)))
    {
        return nullptr;
    }
    ComPtr<ID3D10Blob> g_pPixelShaderBlob;
    if (FAILED(D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &g_pPixelShaderBlob, NULL)))
    {
        return nullptr;
    }
    D3D12_INPUT_ELEMENT_DESC local_layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (UINT)IM_OFFSETOF(ImDrawVert, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
        .pRootSignature = pRootSignature.Get(),
        .VS = {g_pVertexShaderBlob->GetBufferPointer(), g_pVertexShaderBlob->GetBufferSize()},
        .PS = {g_pPixelShaderBlob->GetBufferPointer(), g_pPixelShaderBlob->GetBufferSize()},
        .BlendState = {
            .AlphaToCoverageEnable = false,
            .RenderTarget = {
                {
                    .BlendEnable = true,
                    .SrcBlend = D3D12_BLEND_SRC_ALPHA,
                    .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
                    .BlendOp = D3D12_BLEND_OP_ADD,
                    .SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA,
                    .DestBlendAlpha = D3D12_BLEND_ZERO,
                    .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                    .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
                },
            },
        },
        .SampleMask = UINT_MAX,
        .RasterizerState = {
            .FillMode = D3D12_FILL_MODE_SOLID,
            .CullMode = D3D12_CULL_MODE_NONE,
            .FrontCounterClockwise = FALSE,
            .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
            .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
            .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
            .DepthClipEnable = true,
            .MultisampleEnable = FALSE,
            .AntialiasedLineEnable = FALSE,
            .ForcedSampleCount = 0,
            .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
        },
        .DepthStencilState = {
            .DepthEnable = false,
            .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            .StencilEnable = false,
            .FrontFace = {
                .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            },
            .BackFace = {
                .StencilFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
            },
        },
        .InputLayout = {local_layout, 3},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
        .SampleDesc{
            .Count = 1,
        },
        .NodeMask = 1,
        .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
    };

    ComPtr<ID3D12PipelineState> pPipelineState;
    if (FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPipelineState))))
    {
        return nullptr;
    }
    return pPipelineState;
}

static ComPtr<ID3D12Resource> CreateFontsTexture(const ComPtr<ID3D12Device> &device)
{
    // Build texture atlas
    ImGuiIO &io = ImGui::GetIO();
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    {
        D3D12_HEAP_PROPERTIES props{
            .Type = D3D12_HEAP_TYPE_DEFAULT,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        };

        D3D12_RESOURCE_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment = 0;
        desc.Width = width;
        desc.Height = height;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ComPtr<ID3D12Resource> pTexture;
        device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
                                        D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&pTexture));

        UINT uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
        UINT uploadSize = height * uploadPitch;
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = uploadSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        ComPtr<ID3D12Resource> uploadBuffer;
        HRESULT hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
                                                     D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer));
        IM_ASSERT(SUCCEEDED(hr));

        void *mapped = NULL;
        D3D12_RANGE range = {0, uploadSize};
        hr = uploadBuffer->Map(0, &range, &mapped);
        IM_ASSERT(SUCCEEDED(hr));
        for (int y = 0; y < height; y++)
            memcpy((void *)((uintptr_t)mapped + y * uploadPitch), pixels + y * width * 4, width * 4);
        uploadBuffer->Unmap(0, &range);

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.pResource = uploadBuffer.Get();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcLocation.PlacedFootprint.Footprint.Width = width;
        srcLocation.PlacedFootprint.Footprint.Height = height;
        srcLocation.PlacedFootprint.Footprint.Depth = 1;
        srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.pResource = pTexture.Get();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = 0;

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pTexture.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        ComPtr<ID3D12Fence> fence;
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        IM_ASSERT(SUCCEEDED(hr));

        HANDLE event = CreateEvent(0, 0, 0, 0);
        IM_ASSERT(event != NULL);

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 1;

        ComPtr<ID3D12CommandQueue> cmdQueue;
        hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));
        IM_ASSERT(SUCCEEDED(hr));

        ComPtr<ID3D12CommandAllocator> cmdAlloc;
        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
        IM_ASSERT(SUCCEEDED(hr));

        ComPtr<ID3D12GraphicsCommandList> cmdList;
        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), NULL, IID_PPV_ARGS(&cmdList));
        IM_ASSERT(SUCCEEDED(hr));

        cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
        cmdList->ResourceBarrier(1, &barrier);

        hr = cmdList->Close();
        IM_ASSERT(SUCCEEDED(hr));

        cmdQueue->ExecuteCommandLists(1, (ID3D12CommandList *const *)cmdList.GetAddressOf());
        hr = cmdQueue->Signal(fence.Get(), 1);
        IM_ASSERT(SUCCEEDED(hr));

        fence->SetEventOnCompletion(1, event);
        WaitForSingleObject(event, INFINITE);

        CloseHandle(event);

        return pTexture;
    }
}

