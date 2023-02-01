using System;
using System.Windows.Interop;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows;

namespace EditorUI.Wrappers
{
    public class ModelBrowserWrapper : Wrapper
    {
        public override IntPtr CreateWrapper()
        {
           (ui_window = new ModelBrowser(){ Opacity = 0, Width = 1280, Height = 720 }).Show();
           ui_handle = new WindowInteropHelper(ui_window).Handle;

            return ui_handle;
        }
    }
}
