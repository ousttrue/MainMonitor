#include "Mesh.h"
#include "ResourceItem.h"
#include "CommandList.h"
#include "Material.h"

namespace d12u
{

void Mesh::Setup(class CommandList *commandList)
{
    auto _commandList = commandList->Get();

    //
    // vertexState
    //
    if (!m_vertexBuffer)
    {
        return;
    }
    auto vertexState = m_vertexBuffer->State();
    if (vertexState.State == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        if (vertexState.Upload == UploadStates::Uploaded)
        {
            m_vertexBuffer->EnqueueTransition(commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        }
    }

    //
    // indexState
    //
    if (!m_indexBuffer)
    {
        // //
        // // draw non indexed: deprecated
        // //
        // if (vertexState.Drawable())
        // {
        //     _commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //     _commandList->IASetVertexBuffers(0, 1, &m_vertexBuffer->VertexBufferView());
        //     _commandList->DrawInstanced(m_vertexBuffer->Count(), 1, 0, 0);
        // }
        return;
    }
    auto indexState = m_indexBuffer->State();
    if (indexState.State == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        if (indexState.Upload == UploadStates::Uploaded)
        {
            m_indexBuffer->EnqueueTransition(commandList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        }
    }

    //
    // draw Indexed
    //
    if (vertexState.Drawable() && indexState.Drawable())
    {
        _commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _commandList->IASetVertexBuffers(0, 1, &m_vertexBuffer->VertexBufferView());
        _commandList->IASetIndexBuffer(&m_indexBuffer->IndexBufferView());

        // // draw
        // if (submeshes.empty())
        // {
        //     _commandList->DrawIndexedInstanced(m_indexBuffer->Count(), 1, 0, 0, 0);
        // }
        // else
        // {
        //     int offset = 0;
        //     for (auto &submesh : submeshes)
        //     {
        //         submesh.material->Set(_commandList);
        //         _commandList->DrawIndexedInstanced(submesh.draw_count, 1, 0, 0, 0);
        //         offset += submesh.draw_count;
        //     }
        // }
    }
}

} // namespace d12u