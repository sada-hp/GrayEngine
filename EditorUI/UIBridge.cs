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

        [DllExport]
        public static IntPtr CreateUserInterface(IntPtr owner, uint index)
        {
            if (wrappers[0] == null)
            {
                wrappers[0] = new EditorWrapper();
                wrappers[1] = new ModelBrowserWrapper();
            }

            return wrappers[index].CreateWrapper(owner);
        }

        [DllExport]
        public static void DisplayUserInterface(uint index)
        {
            wrappers[index].DisplayUserInterface();
        }

        [DllExport]
        public static void DestroyUserInterface(uint index)
        {
            wrappers[index].DestroyWrapper();
        }

        [DllExport]
        public static void ParentRenderer(IntPtr value, uint index)
        {
            wrappers[index].ParentRenderer(value);
        }

        [DllExport]
        public static void UpdateChildPosition(uint index)
        {
            wrappers[index].UpdateChildWnd();
        }

        [DllExport]
        public static void UpdateLogger(IntPtr value)
        {
            var input = Marshal.PtrToStringAnsi(value);
            try
            {
                ((MainView)wrappers[0].ui_window).PushIntoLogger(input);
            }
            catch // Can't Access to UI Thread , So Dispatching
            {
                wrappers[0].ui_window.Dispatcher.BeginInvoke((Action)(() =>
                {
                    ((MainView)wrappers[0].ui_window).PushIntoLogger(input);
                }));
            }
        }

        [DllExport]
        public static void UpdateFrameCounter(double frames)
        {
            wrappers[0].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ((MainView)wrappers[0].ui_window).UpdateFrameCounter(frames);
            }));
        }

        [DllExport]
        public static void PassMaterialString(IntPtr value1, IntPtr value2)
        {
            var input1 = Marshal.PtrToStringAnsi(value1);
            var input2 = Marshal.PtrToStringAnsi(value2);

            wrappers[1].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ((ModelBrowser)wrappers[1].ui_window).AddMaterialToTheTable(input1, input2);
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
        public static void RetrieveEntityInfo(IntPtr id, IntPtr name, float X, float Y, float Z)
        {
            wrappers[0].ui_window.Dispatcher.BeginInvoke((Action)(() =>
            {
                ((MainView)wrappers[0].ui_window).RetrieveEntityInfo((int)id, Marshal.PtrToStringAnsi(name), X, Y, Z);
            }));
        }
    }
}