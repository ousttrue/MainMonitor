#include "Application.h"
#include "save_windowplacement.h"
#include <Win32Window.h>
#include <filesystem>
#include <iostream>

#define YAP_ENABLE
#include <YAP.h>

const auto CLASS_NAME = L"ExternalViewerClass";
const auto WINDOW_NAME = L"ExternalViewer";

int main(int argc, char **argv)
{
    screenstate::Win32Window window(CLASS_NAME);
    auto hwnd = window.Create(WINDOW_NAME);
    if (!hwnd)
    {
        std::cerr << "fail to window.Create";
        return 1;
    }

    Application app(argc, argv);

    auto windowconf = std::filesystem::current_path().append("ExternalViewer.window.json").u16string();
    windowplacement::Restore(hwnd, SW_SHOW, (const wchar_t *)windowconf.c_str());
    window.OnDestroy = [hwnd, conf = windowconf]() {
        windowplacement::Save(hwnd, (const wchar_t *)conf.c_str());
    };

    YAP::Init(2, 4, 2048, 16);
    {
        YAP::PushPhase(MainLoop);
        {
            screenstate::ScreenState state;
            while (true)
            {
                YAP::NewFrame();
                {
                    YAP::ScopedSection(Window);
                    if (!window.TryGetInput(&state))
                    {
                        break;
                    }
                }
                {
                    YAP::ScopedSection(Frame);
                    app.OnFrame(hwnd, state);
                }
            }
        }
        YAP::PopPhase();
    }
    YAP::Finish();

    return 0;
}
