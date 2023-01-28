#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <afv-native/hardwareType.h>

namespace afv_unix::util {
        

        inline static std::string getKeyName( const sf::Keyboard::Key key ) {
        switch( key ) {
        default:
                case sf::Keyboard::Unknown:
                        return "Unknown";
                case sf::Keyboard::A:
                        return "A";
                case sf::Keyboard::B:
                        return "B";
                case sf::Keyboard::C:
                        return "C";
                case sf::Keyboard::D:
                        return "D";
                case sf::Keyboard::E:
                        return "E";
                case sf::Keyboard::F:
                        return "F";
                case sf::Keyboard::G:
                        return "G";
                case sf::Keyboard::H:
                        return "H";
                case sf::Keyboard::I:
                        return "I";
                case sf::Keyboard::J:
                        return "J";
                case sf::Keyboard::K:
                        return "K";
                case sf::Keyboard::L:
                        return "L";
                case sf::Keyboard::M:
                        return "M";
                case sf::Keyboard::N:
                        return "N";
                case sf::Keyboard::O:
                        return "O";
                case sf::Keyboard::P:
                        return "P";
                case sf::Keyboard::Q:
                        return "Q";
                case sf::Keyboard::R:
                        return "R";
                case sf::Keyboard::S:
                        return "S";
                case sf::Keyboard::T:
                        return "T";
                case sf::Keyboard::U:
                        return "U";
                case sf::Keyboard::V:
                        return "V";
                case sf::Keyboard::W:
                        return "W";
                case sf::Keyboard::X:
                        return "X";
                case sf::Keyboard::Y:
                        return "Y";
                case sf::Keyboard::Z:
                        return "Z";
                case sf::Keyboard::Num0:
                        return "Num0";
                case sf::Keyboard::Num1:
                        return "Num1";
                case sf::Keyboard::Num2:
                        return "Num2";
                case sf::Keyboard::Num3:
                        return "Num3";
                case sf::Keyboard::Num4:
                        return "Num4";
                case sf::Keyboard::Num5:
                        return "Num5";
                case sf::Keyboard::Num6:
                        return "Num6";
                case sf::Keyboard::Num7:
                        return "Num7";
                case sf::Keyboard::Num8:
                        return "Num8";
                case sf::Keyboard::Num9:
                        return "Num9";
                case sf::Keyboard::Escape:
                        return "Escape";
                case sf::Keyboard::LControl:
                        return "LControl";
                case sf::Keyboard::LShift:
                        return "LShift";
                case sf::Keyboard::LAlt:
                        return "LAlt";
                case sf::Keyboard::LSystem:
                        return "LSystem";
                case sf::Keyboard::RControl:
                        return "RControl";
                case sf::Keyboard::RShift:
                        return "RShift";
                case sf::Keyboard::RAlt:
                        return "RAlt";
                case sf::Keyboard::RSystem:
                        return "RSystem";
                case sf::Keyboard::Menu:
                        return "Menu";
                case sf::Keyboard::LBracket:
                        return "LBracket";
                case sf::Keyboard::RBracket:
                        return "RBracket";
                case sf::Keyboard::SemiColon:
                        return "SemiColon";
                case sf::Keyboard::Comma:
                        return "Comma";
                case sf::Keyboard::Period:
                        return "Period";
                case sf::Keyboard::Quote:
                        return "Quote";
                case sf::Keyboard::Slash:
                        return "Slash";
                case sf::Keyboard::BackSlash:
                        return "BackSlash";
                case sf::Keyboard::Tilde:
                        return "Tilde";
                case sf::Keyboard::Equal:
                        return "Equal";
                case sf::Keyboard::Dash:
                        return "Dash";
                case sf::Keyboard::Space:
                        return "Space";
                case sf::Keyboard::Return:
                        return "Return";
                case sf::Keyboard::BackSpace:
                        return "BackSpace";
                case sf::Keyboard::Tab:
                        return "Tab";
                case sf::Keyboard::PageUp:
                        return "PageUp";
                case sf::Keyboard::PageDown:
                        return "PageDown";
                case sf::Keyboard::End:
                        return "End";
                case sf::Keyboard::Home:
                        return "Home";
                case sf::Keyboard::Insert:
                        return "Insert";
                case sf::Keyboard::Delete:
                        return "Delete";
                case sf::Keyboard::Add:
                        return "Add";
                case sf::Keyboard::Subtract:
                        return "Subtract";
                case sf::Keyboard::Multiply:
                        return "Multiply";
                case sf::Keyboard::Divide:
                        return "Divide";
                case sf::Keyboard::Left:
                        return "Left";
                case sf::Keyboard::Right:
                        return "Right";
                case sf::Keyboard::Up:
                        return "Up";
                case sf::Keyboard::Down:
                        return "Down";
                case sf::Keyboard::Numpad0:
                        return "Numpad0";
                case sf::Keyboard::Numpad1:
                        return "Numpad1";
                case sf::Keyboard::Numpad2:
                        return "Numpad2";
                case sf::Keyboard::Numpad3:
                        return "Numpad3";
                case sf::Keyboard::Numpad4:
                        return "Numpad4";
                case sf::Keyboard::Numpad5:
                        return "Numpad5";
                case sf::Keyboard::Numpad6:
                        return "Numpad6";
                case sf::Keyboard::Numpad7:
                        return "Numpad7";
                case sf::Keyboard::Numpad8:
                        return "Numpad8";
                case sf::Keyboard::Numpad9:
                        return "Numpad9";
                case sf::Keyboard::F1:
                        return "F1";
                case sf::Keyboard::F2:
                        return "F2";
                case sf::Keyboard::F3:
                        return "F3";
                case sf::Keyboard::F4:
                        return "F4";
                case sf::Keyboard::F5:
                        return "F5";
                case sf::Keyboard::F6:
                        return "F6";
                case sf::Keyboard::F7:
                        return "F7";
                case sf::Keyboard::F8:
                        return "F8";
                case sf::Keyboard::F9:
                        return "F9";
                case sf::Keyboard::F10:
                        return "F10";
                case sf::Keyboard::F11:
                        return "F11";
                case sf::Keyboard::F12:
                        return "F12";
                case sf::Keyboard::F13:
                        return "F13";
                case sf::Keyboard::F14:
                        return "F14";
                case sf::Keyboard::F15:
                        return "F15";
                case sf::Keyboard::Pause:
                        return "Pause";
                }
        }

        inline static std::string getHardwareName(const afv_native::HardwareType hardware) {
                switch(hardware) {
                case afv_native::HardwareType::Garex_220:
                        return "Garex 220";
                case afv_native::HardwareType::Rockwell_Collins_2100:
                        return "Rockwell Collins 2100";
                case afv_native::HardwareType::Schmid_ED_137B:
                        return "Schmid ED-137B";
                }

                return "Unknown Hardware";
        }

        inline static std::string ReplaceString(std::string subject, const std::string& search,
        const std::string& replace) {
                size_t pos = 0;
                while ((pos = subject.find(search, pos)) != std::string::npos) {
                        subject.replace(pos, search.length(), replace);
                        pos += replace.length();
                }
                return subject;
        }

        inline void AddUnderLine( ImColor col_ )
        {
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                min.y = max.y;
                ImGui::GetWindowDrawList()->AddLine( min, max, col_, 1.0f );
        }

        //https://stackoverflow.com/questions/64653747/how-to-center-align-text-horizontally
        inline void TextCentered(std::string text) {
                float win_width = ImGui::GetWindowSize().x;
                float text_width = ImGui::CalcTextSize(text.c_str()).x;

                // calculate the indentation that centers the text on one line, relative
                // to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
                float text_indentation = (win_width - text_width) * 0.5f;

                // if text is too long to be drawn on one line, `text_indentation` can
                // become too small or even negative, so we check a minimum indentation
                float min_indentation = 20.0f;
                if (text_indentation <= min_indentation) {
                        text_indentation = min_indentation;
                }

                ImGui::SameLine(text_indentation);
                ImGui::PushTextWrapPos(win_width - text_indentation);
                ImGui::TextWrapped("%s", text.c_str());
                ImGui::PopTextWrapPos();
        }

        inline int PlatformOpen(std::string cmd)
        {
                #ifdef SFML_SYSTEM_WINDOWS
                        cmd = "explorer " + cmd;
                #else
                        cmd = "open " + cmd;
                #endif

                return std::system(cmd.c_str());
        }

        inline void TextURL(std::string name_, std::string URL_)
        {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
                ImGui::TextUnformatted(name_.c_str());
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered())
                {
                        if(ImGui::IsMouseClicked(0))
                        {
                                util::PlatformOpen(URL_);
                        }
                        AddUnderLine( ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
                }
                else
                {
                        AddUnderLine(ImGui::GetStyle().Colors[ImGuiCol_Button]);
                }
        }
}