// Form1.cpp — IMGUI_GOD_SSSTIER editöründen ÜRETİLDİ
// Değişiklikleri USER bölgelerine yazın; editör üretim bloklarını yeniler.
#include "Form1.h"

//== IMGOD:INIT-BEGIN (editör üretimi — elle değiştirmeyin)
void Form1::Render() {
    ImGui::SetNextWindowSize(ImVec2(430, 690), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(u8"Form1", nullptr, 0)) { ImGui::End();
        return; }
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.102f, 0.1255f, 0.1882f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2275f, 0.2902f, 0.4f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::SetCursorPos(ImVec2(24, 24));
    panel1.Begin();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8235f, 0.3294f, 0.06275f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9333f, 0.4784f, 0.1882f, 1.0f));
    ImGui::SetCursorPos(ImVec2(64, 184));
    if (ImGui::Button(u8"Panel İçi Buton##button1", ImVec2(120, 28))) button1_Click();
    ImGui::PopStyleColor(3);
    panel1.End();
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(2);
    ImGui::SetCursorPos(ImVec2(24, 320));
    checkbox1.Checked0 = false;
    if (checkbox1.Draw()) checkbox1_Checked();
    ImGui::SetCursorPos(ImVec2(24, 360));
    if (sliderFloat1.Draw()) sliderFloat1_ValueChanged();
    ImGui::SetCursorPos(ImVec2(24, 400));
    if (combo1.Draw()) combo1_SelectionChanged();
    ImGui::SetCursorPos(ImVec2(24, 440));
    if (listBox1.Draw()) listBox1_SelectionChanged();
    ImGui::SetCursorPos(ImVec2(24, 570));
    progressBar1.Draw();
    ImGui::SetCursorPos(ImVec2(24, 620));
    ImGui::Text(u8"Merhaba Dünya");
    ImGui::End();
}
//== IMGOD:INIT-END

//== IMGOD:HANDLERS-BEGIN
void Form1::panel1_Hovered() {
    // TODO: panel1 → Hovered (Fare üstüne gelince) — kodunuzu yazın
}

void Form1::button1_Click() {
    // TODO: button1 → Click (Tıklandığında) — kodunuzu yazın
}

void Form1::button1_Hovered() {
    // TODO: button1 → Hovered (Fare üstüne gelince) — kodunuzu yazın
}

void Form1::button1_Held() {
    // TODO: button1 → Held (Basılı tutulunca) — kodunuzu yazın
}

void Form1::checkbox1_Checked() {
    // TODO: checkbox1 → Checked (İşaret değişince) — kodunuzu yazın
}

void Form1::sliderFloat1_ValueChanged() {
    // TODO: sliderFloat1 → ValueChanged (Değer kayarken) — kodunuzu yazın
}

void Form1::sliderFloat1_Deactivated() {
    // TODO: sliderFloat1 → Deactivated (Kaydırma bitince) — kodunuzu yazın
}

void Form1::combo1_SelectionChanged() {
    // TODO: combo1 → SelectionChanged (Seçim değişince) — kodunuzu yazın
}

void Form1::listBox1_SelectionChanged() {
    // TODO: listBox1 → SelectionChanged (Seçim değişince) — kodunuzu yazın
}

void Form1::progressBar1_Hovered() {
    // TODO: progressBar1 → Hovered (Fare üstüne gelince) — kodunuzu yazın
}

//== IMGOD:HANDLERS-END

Form1::Form1() {
    panel1.Text = u8"Panel##panel1";
    panel1.Size = ImVec2(370, 270);
    panel1.Border = true;
    panel1.Rounding = 6.0f;
    checkbox1.Text = u8"Seçeneğim##checkbox1";
    sliderFloat1.Text = u8"Değer##sliderFloat1";
    sliderFloat1.Value = 40.0f;
    sliderFloat1.Min = 0.0f;
    sliderFloat1.Max = 100.0f;
    sliderFloat1.Format = u8"%.1f";
    sliderFloat1.Width = 280;
    combo1.Text = u8"Seçim##combo1";
    combo1.Items = { u8"Seçenek 1", u8"Seçenek 2", u8"Seçenek 3" };
    combo1.Selected = 0;
    combo1.Width = 300;
    listBox1.Text = u8"Liste##listBox1";
    listBox1.Items = { u8"Öğe 1", u8"Öğe 2", u8"Öğe 3" };
    listBox1.Selected = 0;
    listBox1.HeightItems = 5;
    listBox1.Size = ImVec2(240, 110);
    progressBar1.Value = 0.7f;
    progressBar1.Overlay = u8"";
    progressBar1.Size = ImVec2(200, 18);
}
Form1::~Form1() {}
void Form1::OnLoad() {
    // TODO: form yüklendiğinde çalışacak kod
}
