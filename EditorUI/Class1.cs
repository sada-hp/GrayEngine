using System;
using System.Windows.Interop;
using RGiesecke.DllExport;
using System.Threading;

namespace EditorUI
{
    public class Class1
    {
        public static MainView mainview_ui;
        public static Thread gui_thread;
        public static IntPtr mainview_handle = IntPtr.Zero;

        [DllExport]
        public static IntPtr CreateUserInterface(IntPtr pointer) // Multi-Threaded Version
        {
            gui_thread = new Thread(() =>
            {
                mainview_ui = new MainView(pointer)
                { Opacity = 0, Width = 0, Height = 0 };
                mainview_ui.Show();
                mainview_handle = new WindowInteropHelper(mainview_ui).Handle;
                System.Windows.Threading.Dispatcher.Run();
            });
            gui_thread.SetApartmentState(ApartmentState.STA); // STA Thread Initialization
            gui_thread.Start();

            while (mainview_handle == IntPtr.Zero) { }
            return mainview_handle;
        }

        [DllExport]
        public static void DisplayUserInterface() // Multi-Threaded Version
        {
            try
            {
                mainview_ui.Opacity = 1;
            }
            catch // Can't Access to UI Thread , So Dispatching
            {
                mainview_ui.Dispatcher.BeginInvoke((Action)(() =>
                {
                    mainview_ui.Opacity = 1;
                }));
            }
        }

        [DllExport]
        public static void DestroyUserInterface() // Multi-Threaded Version
        {
            try
            {
                mainview_ui.Close();
            }
            catch // Can't Access to UI Thread , So Dispatching
            {
                mainview_ui.Dispatcher.BeginInvoke((Action)(() =>
                {
                    mainview_ui.Close();
                }));
            }
        }
    }
}
