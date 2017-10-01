#pragma once

#include <cstdio>
#include <ctime>

#include <imgui.h>
#include <imgui_internal.h>
#include <ImwWindow.h>

#include "External\IconsMaterialDesign.h"

class Output : public ImWindow::ImwWindow
{
public:
	Output()
	{
		SetTitle(ICON_MD_FORMAT_ALIGN_LEFT " Output");
	}
	void Clear() { Buf.clear(); LineOffsets.clear(); }

	void AddLog(const char* fmt, ...)
	{
		std::time_t rawtime;
		std::tm* timeinfo;
		char buffer[80];

		std::time(&rawtime);
		timeinfo = std::localtime(&rawtime);

		std::strftime(buffer, 80, "[%H:%M:%S] ", timeinfo);

		Buf.append(buffer);

		int old_size = Buf.size();
		va_list args;
		va_start(args, fmt);
		Buf.appendv(fmt, args);
		va_end(args);
		for (int new_size = Buf.size(); old_size < new_size; old_size++)
			if (Buf[old_size] == '\n')
				LineOffsets.push_back(old_size);
		ScrollToBottom = true;
	}

	virtual void OnGui() override
	{
		if (ImGui::Button("Clear")) Clear();
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		Filter.Draw("Filter", 250.0f);
		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		if (copy) ImGui::LogToClipboard();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const char* buf_begin = Buf.begin();
		const char* line = buf_begin;
		for (int line_no = 0; line != NULL; line_no++)
		{
			const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
			if (Filter.PassFilter(line, line_end)) {
				ImVec4 col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // A better implementation may store a type per-item. For the sample let's just parse the text.
				if (sstrstr(line, "[error]", line_end - line)) col = ImColor(1.f, 0.659f, 0.561f, 1.f);
					
				ImGui::PushStyleColor(ImGuiCol_Text, col);
				ImGui::TextUnformatted(line, line_end);
				ImGui::PopStyleColor();

			}
			line = line_end && line_end[1] ? line_end + 1 : NULL;
		}
		ImGui::PopStyleVar(1);

		if (ScrollToBottom)
			ImGui::SetScrollHere(1.0f);
		ScrollToBottom = false;
		ImGui::EndChild();
	}
private:
	ImGuiTextBuffer Buf;
	ImGuiTextFilter Filter;
	ImVector<int> LineOffsets;        // Index to lines offset
	bool ScrollToBottom;


	const char *sstrstr(const char *haystack, const char *needle, size_t length)
	{
		size_t needle_length = strlen(needle);
		size_t i;

		for (i = 0; i < length; i++)
		{
			if (i + needle_length > length)
			{
				return NULL;
			}

			if (strncmp(&haystack[i], needle, needle_length) == 0)
			{
				return &haystack[i];
			}
		}
		return NULL;
	}
};