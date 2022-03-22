using System;
using System.Windows.Interop;
using RGiesecke.DllExport;
using System.Threading;
using System.Runtime.InteropServices;

namespace EditorUI
{
    public class UIBridge
    {
        public static MainView mainview_ui;
        public static Thread gui_thread;
        public static IntPtr mainview_handle = IntPtr.Zero;

        [DllExport]
        public static IntPtr CreateUserInterface(IntPtr pointer) // Multi-Threaded Version
        {
            gui_thread = new Thread(() =>
            {
                if (mainview_handle == IntPtr.Zero)
                {
                    mainview_ui = new MainView(pointer)
                    { Opacity = 0, Width = 0, Height = 0 };
                    mainview_ui.Show();
                    mainview_handle = new WindowInteropHelper(mainview_ui).Handle;
                }
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

        [DllExport]
        public static void UpdateLogger(IntPtr value) // Multi-Threaded Version
        {
            var input = Marshal.PtrToStringAnsi(value);
            try
            {
                mainview_ui.PushIntoLogger(input);
            }
            catch // Can't Access to UI Thread , So Dispatching
            {
                mainview_ui.Dispatcher.BeginInvoke((Action)(() =>
                {
                    mainview_ui.PushIntoLogger(input);
                }));
            }
        }

        [DllExport]
        public static void ParentRenderer(IntPtr value) // Multi-Threaded Version
        {
            try
            {
                mainview_ui.ParentRender(value);
            }
            catch // Can't Access to UI Thread , So Dispatching
            {
                mainview_ui.Dispatcher.BeginInvoke((Action)(() =>
                {
                    mainview_ui.ParentRender(value);
                }));
            }
        }

        [DllExport]
        public static void UpdateChildPosition()
        {
            try
            {
                mainview_ui.UpdateChildPosition();
            }
            catch // Can't Access to UI Thread , So Dispatching
            {
                mainview_ui.Dispatcher.BeginInvoke((Action)(() =>
                {
                    mainview_ui.UpdateChildPosition();
                }));
            }
        }
    }
}
