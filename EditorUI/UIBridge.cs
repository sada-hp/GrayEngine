﻿using System;
using System.Windows.Interop;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows;
using EditorUI.Wrappers;

namespace EditorUI
{
    public class UIBridge
    {
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LogMessage(IntPtr msg);
        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hWnd, int wMsg, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")]
        public static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);
        [DllImport("user32.dll")]
        public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LoadModelFile(IntPtr mesh_path);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AssignTextures(IntPtr image_path);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LoadObject(IntPtr mesh_path, IntPtr tex_path);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AddToTheScene(IntPtr mesh_path);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void CreateModelFile(IntPtr mesh_path, IntPtr textures);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void CloseContext();
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AddEntity();
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void GetEntityInfo(IntPtr ID);

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void GetEntitiesList();

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void InitModelBrowser();

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void UpdateEntityProperty(int ID, IntPtr property, IntPtr value);

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void UpdateSkybox(IntPtr East, IntPtr West, IntPtr Top, IntPtr Bottom, IntPtr North, IntPtr South);

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SaveScreenshot(IntPtr filepath);

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void TogglePhysics();

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AddNewEntityProperty(int ID, IntPtr property);

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SaveScene(IntPtr path);

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LoadScene(IntPtr path);

        public static Wrapper[] wrappers = new Wrapper[2];
        public static Thread uThread;
        static int WrappersCount = 0;

        [DllExport]
        public static IntPtr CreateUserInterface(uint index)
        {
            if (uThread == null)
            {
                uThread = new Thread(() => {
                    if (wrappers[0] == null)
                    {
                        wrappers[0] = new EditorWrapper();
                    }
                    if (wrappers[1] == null)
                    {
                        wrappers[1] = new ModelBrowserWrapper();
                    }
                    System.Windows.Threading.Dispatcher.Run();
                });

                uThread.SetApartmentState(ApartmentState.STA);
                uThread.Start();

                while (wrappers[index] == null) { };
            }
            WrappersCount++;


            return wrappers[index].CreateWrapper();
        }

        [DllExport]
        public static void DisplayUserInterface(uint index)
        {
            wrappers[index].ui_window.Dispatcher.Invoke((Action)(() =>
            {
                wrappers[index].DisplayUserInterface();
            }));
        }

        [DllExport]
        public static void DestroyUserInterface(uint index)
        {
            wrappers[index].ui_window.Dispatcher.Invoke((Action)(() =>
            {
                wrappers[index].DestroyWrapper();
            }));

            wrappers[index].ui_window.Dispatcher.InvokeShutdown();
            WrappersCount--;

            if (WrappersCount <= 0)
            {
                uThread.Abort();
                uThread = null;
            }
            GC.Collect();
        }

        [DllExport]
        public static void ParentRenderer(IntPtr value, uint index)
        {
            wrappers[index].ui_window.Dispatcher.Invoke((Action)(() =>
            {
                wrappers[index].ParentRenderer(value);
            }));
        }

        [DllExport]
        public static void UpdateChildPosition(uint index)
        {
            wrappers[index].ui_window.Dispatcher.Invoke((Action)(() =>
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
            wrappers[index].ui_window.Dispatcher.Invoke((Action)(() =>
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