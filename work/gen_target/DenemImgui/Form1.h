// Form1.h — IMGUI_GOD_SSSTIER editöründen ÜRETİLDİ
#pragma once
#include "Global.h"

struct Form1 : IGOD::FormBase {
    // button1 : Button — tasarım: Panel İçi Buton
    IGOD::Panel panel1;
    // button1 : Button — tasarım: Panel İçi Buton
    IGOD::Checkbox checkbox1;   // tasarım: Seçeneğim
    IGOD::SliderFloat sliderFloat1;   // tasarım: Değer
    IGOD::Combo combo1;
    IGOD::ProgressBar progressBar1;
    void panel1_Hovered();   // panel1: Fare üstüne gelince
    void button1_Click();   // button1: Tıklandığında
    void button1_Hovered();   // button1: Fare üstüne gelince
    void button1_Held();   // button1: Basılı tutulunca
    void checkbox1_Checked();   // checkbox1: İşaret değişince
    void sliderFloat1_ValueChanged();   // sliderFloat1: Değer kayarken
    void sliderFloat1_Deactivated();   // sliderFloat1: Kaydırma bitince
    void combo1_SelectionChanged();   // combo1: Seçim değişince
    void progressBar1_Hovered();   // progressBar1: Fare üstüne gelince
    void OnLoad() override;
    Form1();
    ~Form1() override;   // vtable bu TU'da bağlanır
    void Render() override;
};
