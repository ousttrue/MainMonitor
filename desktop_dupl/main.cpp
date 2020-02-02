#include "overlay.h"
#include "duplicator.h"

const auto OVERLAY_KEY = "desktop_dupl";
const auto OVERLAY_LABEL = "desktop dupl";

int main(int argc, char **argv)
{
    // openvr overlay
    Overlay overlay;
    if (!overlay.Initialize(OVERLAY_KEY, OVERLAY_LABEL))
    {
        return 1;
    }

    // create shared handle for desktop texture
    DesktopDuplicator dupl;
    auto handle = dupl.CreateDuplAndSharedHandle();
    if (!handle)
    {
        return 2;
    }
    overlay.SetSharedHanle(handle);

    // main loop
    bool isActive;
    while (overlay.Loop(&isActive))
    {
        if (isActive)
        {
            // update shared texture
            if (!dupl.Duplicate())
            {
                // error
                break;
            }
        }
        else
        {
            // wait
            dupl.Sleep(100);
        }
    }

    return 0;
}
