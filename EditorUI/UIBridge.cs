using System;
using System.Windows.Interop;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows;
using EditorUI.Wrappers;

namespace EditorUI
{
    public class UIBridge
    {
        public static Wrapper[] wrappers = new Wrapper[2];
        public static Thread uThread;

        [DllExport]
        public static IntPtr CreateUserInterface(uint index)
        {
            uThread = new Thread(() => { 
                if (wrappers[0] == null)
                {
                    wrappers[0] = new EditorWrapper();
                    wrappers[1] = new ModelBrowserWrapper();
                }
                System.Windows.Threading.Dispatcher.Run();
            });
            uThread.SetApartmentState(ApartmentState.STA);
            uThread.Start();
            while (wrappers[1] == null && wrappers[0] == null) { };

            return wrappers[index].CreateWrapper();
        }

        [DllExport]
        public static void DisplayUserInterface(uint index)
        {
            wrappers[index].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                wrappers[index].DisplayUserInterface();
            }));
        }

        [DllExport]
        public static void DestroyUserInterface(uint index)
        {
            wrappers[index].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                wrappers[index].DestroyWrapper();
            }));
        }

        [DllExport]
        public static void ParentRenderer(IntPtr value, uint index)
        {
            wrappers[index].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                wrappers[index].ParentRenderer(value);
            }));
        }

        [DllExport]
        public static void UpdateChildPosition(uint index)
        {
            wrappers[index].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                wrappers[index].UpdateChildWnd();
            }));
        }

        [DllExport]
        public static void PassMaterialString(IntPtr value1, IntPtr value2, IntPtr redraw)
        {
            var input1 = Marshal.PtrToStringAnsi(value1);
            var input2 = Marshal.PtrToStringAnsi(value2);

            wrappers[1].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ((ModelBrowser)wrappers[1].ui_window).AddMaterialToTheTable(input1, input2, (int)redraw);
            }));
        }

        [DllExport]
        public static void UpdateEntity(IntPtr id, IntPtr name)
        {
            wrappers[0].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ((MainView)wrappers[0].ui_window).UpdateEntity((int)id, Marshal.PtrToStringAnsi(name));
            }));
        }

        [DllExport]
        public static void RemoveEntity(IntPtr id)
        {
            wrappers[0].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ((MainView)wrappers[0].ui_window).RemoveEntity((int)id);
            }));
        }

        [DllExport]
        public static void SetInputMode(uint index, int allow)
        {
            wrappers[index].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                wrappers[index].IsInputAllowed(allow);
            }));
        }

        [DllExport]
        public static void RecieveInfoChunk(int type, IntPtr name, IntPtr value)
        {
            wrappers[0].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ((MainView)wrappers[0].ui_window).UpdateInfo(type, Marshal.PtrToStringAnsi(name), Marshal.PtrToStringAnsi(value));
            }));
        }
    }
}