#pragma once
#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <limits>

namespace vector_audio::style {
inline static void button_yellow()
{
    ImGui::PushStyleColor(
        ImGuiCol_Button, ImColor::HSV(1 / 7.0F, 0.6F, 0.6F).Value);
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered, ImColor::HSV(1 / 7.0F, 0.7F, 0.7F).Value);
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive, ImColor::HSV(1 / 7.0F, 0.8F, 0.8F).Value);
};

inline static void button_blue()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImColor(34, 107, 201).Value);
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered, ImVec4(0.12F, 0.20F, 0.28F, 1.00F));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive, ImVec4(0.06F, 0.53F, 0.98F, 1.00F));
};

inline static void button_green()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImColor(30, 140, 45).Value);
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered, ImColor::HSV(2 / 7.0F, 0.7F, 0.7F).Value);
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive, ImColor::HSV(2 / 7.0F, 0.8F, 0.8F).Value);
};

inline static void button_purple()
{
    ImGui::PushStyleColor(
        ImGuiCol_Button, ImColor::HSV(5 / 7.0F, 0.6F, 0.6F).Value);
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered, ImColor::HSV(5 / 7.0F, 0.7F, 0.7F).Value);
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive, ImColor::HSV(5 / 7.0F, 0.8F, 0.8F).Value);
};

inline static void button_reset_colour() { ImGui::PopStyleColor(3); };

inline static void push_disabled_on(bool flag)
{
    if (!flag)
        return;

    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5F);
};

inline static void pop_disabled_on(bool flag)
{
    if (!flag)
        return;

    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
};

inline static float Saturate(float value)
{
    return std::min(std::max(value, 0.0F), 1.0F);
}

inline static void dualVUMeter(float fractionVu, float fractionPeak,
    const ImVec2& size_arg, ImColor vuColor, ImColor peakColor)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(size_arg, ImGui::CalcItemWidth(),
        g.FontSize + style.FramePadding.y * 2.0F);
    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, 0))
        return;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Render

    fractionVu = Saturate(fractionVu);
    fractionPeak = Saturate(fractionPeak);
    ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg),
        true, style.FrameRounding);
    bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));

    ImVec2 fillBr = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fractionPeak), bb.Max.y);
    ImGui::RenderRectFilledRangeH(window->DrawList, bb, peakColor, 0.0F,
        fractionPeak, style.FrameRounding);

    fillBr = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fractionVu), bb.Max.y);
    ImGui::RenderRectFilledRangeH(
        window->DrawList, bb, vuColor, 0.0F, fractionVu, style.FrameRounding);
}

inline static void apply_style()
{
    ImGui::GetStyle().FrameRounding = 4.0F;
    ImGui::GetStyle().GrabRounding = 4.0F;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95F, 0.96F, 0.98F, 1.00F);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36F, 0.42F, 0.47F, 1.00F);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11F, 0.15F, 0.17F, 1.00F);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15F, 0.18F, 0.22F, 1.00F);
    colors[ImGuiCol_PopupBg] = ImVec4(0.11F, 0.15F, 0.17F, 1.00F);
    colors[ImGuiCol_Border] = ImVec4(0.08F, 0.10F, 0.12F, 1.00F);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00F, 0.00F, 0.00F, 0.00F);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20F, 0.25F, 0.29F, 1.00F);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12F, 0.20F, 0.28F, 1.00F);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09F, 0.12F, 0.14F, 1.00F);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09F, 0.12F, 0.14F, 0.65F);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08F, 0.10F, 0.12F, 1.00F);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00F, 0.00F, 0.00F, 0.51F);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15F, 0.18F, 0.22F, 1.00F);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02F, 0.02F, 0.02F, 0.39F);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20F, 0.25F, 0.29F, 1.00F);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18F, 0.22F, 0.25F, 1.00F);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09F, 0.21F, 0.31F, 1.00F);
    colors[ImGuiCol_CheckMark] = ImVec4(0.28F, 0.56F, 1.00F, 1.00F);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.28F, 0.56F, 1.00F, 1.00F);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37F, 0.61F, 1.00F, 1.00F);
    colors[ImGuiCol_Button] = ImVec4(0.20F, 0.25F, 0.29F, 1.00F);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28F, 0.56F, 1.00F, 1.00F);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06F, 0.53F, 0.98F, 1.00F);
    colors[ImGuiCol_Header] = ImVec4(0.20F, 0.25F, 0.29F, 0.55F);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26F, 0.59F, 0.98F, 0.80F);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26F, 0.59F, 0.98F, 1.00F);
    colors[ImGuiCol_Separator] = ImVec4(0.20F, 0.25F, 0.29F, 1.00F);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10F, 0.40F, 0.75F, 0.78F);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10F, 0.40F, 0.75F, 1.00F);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26F, 0.59F, 0.98F, 0.25F);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26F, 0.59F, 0.98F, 0.67F);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26F, 0.59F, 0.98F, 0.95F);
    colors[ImGuiCol_Tab] = ImVec4(0.11F, 0.15F, 0.17F, 1.00F);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26F, 0.59F, 0.98F, 0.80F);
    colors[ImGuiCol_TabActive] = ImVec4(0.20F, 0.25F, 0.29F, 1.00F);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.11F, 0.15F, 0.17F, 1.00F);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11F, 0.15F, 0.17F, 1.00F);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61F, 0.61F, 0.61F, 1.00F);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00F, 0.43F, 0.35F, 1.00F);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90F, 0.70F, 0.00F, 1.00F);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00F, 0.60F, 0.00F, 1.00F);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26F, 0.59F, 0.98F, 0.35F);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00F, 1.00F, 0.00F, 0.90F);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26F, 0.59F, 0.98F, 1.00F);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00F, 1.00F, 1.00F, 0.70F);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80F, 0.80F, 0.80F, 0.20F);
}
}
