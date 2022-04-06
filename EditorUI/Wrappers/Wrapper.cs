using System;
using System.Windows.Interop;
using RGiesecke.DllExport;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows;
using EditorUI;

namespace EditorUI.Wrappers
{
    public abstract class Wrapper
    {
        internal Window ui_window;
        internal Thread ui_thread;
        internal IntPtr ui_handle;

        public abstract IntPtr CreateWrapper(IntPtr owner);
        public void DestroyWrapper()
        {
            ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ui_window.Close();
            }));

            ui_handle = IntPtr.Zero;
        }

        public void DisplayUserInterface() // Multi-Threaded Version
        {
            ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ui_window.Opacity = 1;
            }));
        }

        public void ParentRenderer(IntPtr value)
        {
            ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ((EditorWindow)ui_window).ParentRender(value);
            }));
        }

        public void UpdateChildWnd()
        {
            ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ((EditorWindow)ui_window).UpdateChildPosition();
            }));
        }
    }
}
