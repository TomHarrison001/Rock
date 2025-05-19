/** \file ui.hpp */

#pragma once

#include <string>

#include "core/device.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#define IMGUI_ENABLE_FREETYPE  // higher quality font rendering
#include "imgui/imgui_internal.h"

struct Console
{
    ImGuiTextBuffer m_buffer;
    ImGuiTextFilter m_filter;
    ImVector<int> m_lineOffsets;
    bool m_autoScroll;

    Console()
    {
        m_autoScroll = true;
        clear();
    }

    void clear()
    {
        m_buffer.clear();
        m_lineOffsets.clear();
        m_lineOffsets.push_back(0);
    }

    void addLog(const char* message)
    {
        int oldSize = m_buffer.size();
        m_buffer.append(message);
        for (int newSize = m_buffer.size(); oldSize < newSize; oldSize++)
        {
            if (m_buffer[oldSize] == '\n')
                m_lineOffsets.push_back(oldSize + 1);
        }
    }

    void draw(const char* title)
    {
        if (!ImGui::Begin(title))
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &m_autoScroll);
            ImGui::EndPopup();
        }

        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        m_filter.Draw("Filter", -200.f);
        ImGui::SameLine();
        bool add = ImGui::Button("Add");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::Separator();

        if (ImGui::BeginChild("Scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (add)
            {
                for (int i = 0; i < 10; i++)
                {
                    static int counter = 1;
                    std::string num = std::to_string(counter);
                    num = "[Console]: Successfully output log " + num + ".\n";
                    const char* message = num.c_str();
                    addLog(message);
                    counter++;
                }
            }
            if (copy)
                ImGui::LogToClipboard();
            if (clear)
                this->clear();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            if (m_filter.IsActive())
            {
                for (int line = 0; line < m_lineOffsets.size(); line++)
                {
                    const char* lineStart = m_buffer.begin() + m_lineOffsets[line];
                    const char* lineEnd = (line + 1 < m_lineOffsets.Size ? (m_buffer.begin() + m_lineOffsets[line + 1] - 1) : m_buffer.end());
                    if (m_filter.PassFilter(lineStart, lineEnd))
                        ImGui::TextUnformatted(lineStart, lineEnd);
                }
            }
            else
            {
                ImGuiListClipper clipper;
                clipper.Begin(m_lineOffsets.Size);
                while (clipper.Step())
                {
                    for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; line++)
                    {
                        const char* lineStart = m_buffer.begin() + m_lineOffsets[line];
                        const char* lineEnd = (line + 1 < m_lineOffsets.Size ? (m_buffer.begin() + m_lineOffsets[line + 1] - 1) : m_buffer.end());
                        ImGui::TextUnformatted(lineStart, lineEnd);
                    }
                }
                clipper.End();
            }
            ImGui::PopStyleVar();

            if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.f);
        }
        ImGui::EndChild();
        ImGui::End();
    }
};

static void createMainMenuBar(Device* device)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            if (ImGui::MenuItem("Save", "Ctrl+S")) {}
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {}
            ImGui::Separator();
            if (ImGui::BeginMenu("Options"))
            {
                static float f = 0.5f;
                static int n = 0;
                ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
                ImGui::InputFloat("Input", &f, 0.1f);
                ImGui::Combo("Combo", &n, "Yes\0Maybe\0No\0\0");
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                std::cout << "[EventSystem] Exiting..." << std::endl;
                device->closeWindow();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {} // Disabled item
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

static void createOverlay(float fps)
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    const float padding_x = 10.0f, padding_y = 10.f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos = ImVec2(work_pos.x + work_size.x - padding_x,
        work_pos.y + work_size.y - padding_y);
    ImVec2 window_pos_pivot = ImVec2(1.f, 1.f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowBgAlpha(0.7f); // Transparent background

    if (ImGui::Begin("FPS overlay", (bool*)true, window_flags))
    {
        ImGui::Text("%.1f FPS (%.3f ms/frame)", fps, 1000.f / fps);
        ImGui::End();
    }
}
