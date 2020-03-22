#include "Application.h"
#include "save_windowplacement.h"
#include "frame_metrics.h"
#include <Win32Window.h>
#include <filesystem>
#include <iostream>

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

    {
        {
            screenstate::ScreenState state;
            while (true)
            {
                frame_metrics::new_frame_delta_seconds(state.DeltaSeconds);
                frame_metrics::scoped("frame");
                {
                    frame_metrics::scoped("window");
                    if (!window.TryGetInput(&state))
                    {
                        break;
                    }
                }
                {
                    frame_metrics::scoped("app");
                    app.OnFrame(hwnd, state);
                }
            }
        }
    }

    return 0;
}
