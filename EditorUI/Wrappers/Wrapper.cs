using System;
using System.Windows.Interop;
using System.Threading;
using System.Windows.Threading;
using System.Runtime.InteropServices;
using System.Windows;
using EditorUI;

namespace EditorUI.Wrappers
{
    public abstract class Wrapper
    {
        internal Window ui_window;
        internal IntPtr ui_handle;

        public abstract IntPtr CreateWrapper();
        public void DestroyWrapper()
        {
            //ui_window.Close();
            ui_handle = IntPtr.Zero;
        }

        public void DisplayUserInterface()
        {
            ui_window.Opacity = 1;
        }

        public void ParentRenderer(IntPtr value)
        {
            ((EditorWindow)ui_window).ParentRender(value);
        }

        public void UpdateChildWnd()
        {
            ((EditorWindow)ui_window).UpdateChildPosition();
        }

        public void IsInputAllowed(int allow)
        {
            if (allow == 0)
                ui_window.IsEnabled = false;
            else
                ui_window.IsEnabled = true;
        }
    }
}
