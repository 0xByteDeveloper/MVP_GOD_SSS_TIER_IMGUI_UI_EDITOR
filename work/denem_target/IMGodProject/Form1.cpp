// Form1.cpp — IMGUI_GOD_SSSTIER editöründen ÜRETİLDİ
// Değişiklikleri USER bölgelerine yazın; editör üretim bloklarını yeniler.
#include "Form1.h"

//== IMGOD:INIT-BEGIN (editör üretimi — elle değiştirmeyin)
void Form1::Render() {
    ImGui::SetNextWindowSize(ImVec2(430, 520), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(u8"Form1", nullptr, 0)) { ImGui::End();
        return; }
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8235f, 0.3294f, 0.06275f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9333f, 0.4784f, 0.1882f, 1.0f));
    ImGui::SetCursorPos(ImVec2(60, 80));
    if (ImGui::Button(u8"Merhaba Dünya##button1", ImVec2(120, 28))) button1_Click();
    ImGui::PopStyleColor(3);
    ImGui::SetCursorPos(ImVec2(60, 130));
    checkbox1.Checked0 = false;
    if (checkbox1.Draw()) checkbox1_Checked();
    ImGui::SetCursorPos(ImVec2(60, 170));
    if (sliderFloat1.Draw()) sliderFloat1_ValueChanged();
    ImGui::SetCursorPos(ImVec2(60, 220));
    if (inputText1.Draw()) inputText1_TextChanged();
    ImGui::SetCursorPos(ImVec2(60, 260));
    if (combo1.Draw()) combo1_SelectionChanged();
    ImGui::SetCursorPos(ImVec2(60, 310));
    progressBar1.Draw();
    ImGui::SetCursorPos(ImVec2(60, 350));
    ImGui::Text(u8"Etiket");
    ImGui::End();
}
//== IMGOD:INIT-END

//== IMGOD:HANDLERS-BEGIN
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

void Form1::inputText1_TextChanged() {
    // TODO: inputText1 → TextChanged (Yazı değişince) — kodunuzu yazın
}

void Form1::inputText1_EnterPressed() {
    // TODO: inputText1 → EnterPressed (Enter'a basılınca) — kodunuzu yazın
}

void Form1::combo1_SelectionChanged() {
    // TODO: combo1 → SelectionChanged (Seçim değişince) — kodunuzu yazın
}

void Form1::progressBar1_Hovered() {
    // TODO: progressBar1 → Hovered (Fare üstüne gelince) — kodunuzu yazın
}

//== IMGOD:HANDLERS-END

Form1::Form1() {
    checkbox1.Text = u8"Seçeneğim##checkbox1";
    sliderFloat1.Text = u8"Değer##sliderFloat1";
    sliderFloat1.Value = 40.0f;
    sliderFloat1.Min = 0.0f;
    sliderFloat1.Max = 100.0f;
    sliderFloat1.Format = u8"%.1f";
    inputText1.Text = u8"Yazı##inputText1";
    inputText1.Value = u8"Deneme metni";
    inputText1.Hint = u8"İpucu...";
    inputText1.Password = false;
    inputText1.ReadOnly = false;
    combo1.Text = u8"Seçim##combo1";
    combo1.Items = { u8"Seçenek 1", u8"Seçenek 2", u8"Seçenek 3" };
    combo1.Selected = 0;
    progressBar1.Value = 0.5f;
    progressBar1.Overlay = u8"";
    progressBar1.Size = ImVec2(200, 18);
}
Form1::~Form1() {}
void Form1::OnLoad() {
    // TODO: form yüklendiğinde çalışacak kod
}
