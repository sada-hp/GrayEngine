using System;
using System.Windows.Interop;
using RGiesecke.DllExport;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows;

namespace EditorUI.Wrappers
{
    public class EditorWrapper : Wrapper
    {
        public override IntPtr CreateWrapper(IntPtr owner)
        {
            ui_thread = new Thread(() =>
            {
                if (ui_handle == IntPtr.Zero)
                {
                    ui_window = new MainView(owner)
                    { Opacity = 0, Width = 0, Height = 0 };
                    ui_window.Show();
                    ui_handle = new WindowInteropHelper(ui_window).Handle;
                }
                System.Windows.Threading.Dispatcher.Run();
            });
            ui_thread.SetApartmentState(ApartmentState.STA); // STA Thread Initialization
            ui_thread.Start();

            while (ui_handle == IntPtr.Zero) { }
            return ui_handle;
        }
    }
}
