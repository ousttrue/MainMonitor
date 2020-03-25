#include "GuiView.h"
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

// frame_padding < 0: uses FramePadding from style (default)
// frame_padding = 0: no framing
// frame_padding > 0: set framing size
// The color used are the button colors.
static bool ViewButton(void *p, ImTextureID user_texture_id, const ImVec2 &size, const ImVec2 &uv0 = ImVec2(0, 0), const ImVec2 &uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4 &bg_col = ImVec4(0, 0, 0, 0), const ImVec4 &tint_col = ImVec4(1, 1, 1, 1))
{
    using namespace ImGui;

    ImGuiWindow *window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;

    // Default to using texture ID as ID. User can still push string/integer prefixes.
    // We could hash the size/uv to create a unique ID but that would prevent the user from animating UV.
    PushID((void *)(intptr_t)p);
    const ImGuiID id = window->GetID("#image");
    PopID();

    const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
    const ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);
    ItemSize(bb);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    auto flags = ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    RenderNavHighlight(bb, id);
    RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, style.FrameRounding));
    if (bg_col.w > 0.0f)
        window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));
    window->DrawList->AddImage(user_texture_id, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(tint_col));

    return pressed;
}

namespace gui
{

bool View(hierarchy::SceneView *view, const screenstate::ScreenState &state, size_t textureID,
          screenstate::ScreenState *viewState)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool isOpen = ImGui::Begin("view", nullptr,
                               ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (isOpen)
    {
        auto size = ImGui::GetContentRegionAvail();
        auto pos = ImGui::GetWindowPos();
        auto frameHeight = ImGui::GetFrameHeight();

        *viewState = state;
        viewState->Width = (int)size.x;
        viewState->Height = (int)size.y;
        viewState->MouseX = state.MouseX - (int)pos.x;
        viewState->MouseY = state.MouseY - (int)pos.y - (int)frameHeight;

        ImGui::Checkbox("grid", &view->ShowGrid);
        ImGui::SameLine();
        ImGui::Checkbox("openvr", &view->ShowVR);
        ImGui::SameLine();
        ImGui::Checkbox("gizmo", &view->ShowGizmo);
        ImGui::ColorEdit3("clear", view->ClearColor.data());

        ViewButton(view, (ImTextureID)textureID, size, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 0);
        // update camera
        if (!ImGui::IsWindowHovered())
        {
            viewState->Unset(screenstate::MouseButtonFlags::WheelMinus);
            viewState->Unset(screenstate::MouseButtonFlags::WheelPlus);
        }

        if (ImGui::IsItemActive())
        {
            // LOGD << "active";
        }
        else
        {
            // viewState->Unset(screenstate::MouseButtonFlags::LeftDown);
            viewState->Unset(screenstate::MouseButtonFlags::RightDown);
            viewState->Unset(screenstate::MouseButtonFlags::MiddleDown);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();

    return isOpen;
}

} // namespace gui
