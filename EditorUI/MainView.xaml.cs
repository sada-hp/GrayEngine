using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using System.Windows.Forms;

namespace EditorUI
{
    public partial class MainView : Window, EditorWindow
    {
        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hWnd, int wMsg, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")]
        public static extern int FindWindow(string lpClassName, String lpWindowName);
        [DllImport("user32.dll")]
        static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);
        [DllImport("user32.dll")]
        static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        IntPtr pOwner;
        public Form viewport = new Form();
        IntPtr child_hwnd;
        System.Windows.Forms.Panel panel = new System.Windows.Forms.Panel();

        public MainView()
        {
            InitializeComponent();
            panel.CreateControl();
            panel.Dock = DockStyle.Fill;
            panel.BackColor = System.Drawing.Color.Black;
            panel.BorderStyle = BorderStyle.None;
            panel.Margin = new System.Windows.Forms.Padding(0);
            FormHost.Child = panel;
        }

        public MainView(IntPtr p)
        {
            pOwner = p;
            InitializeComponent();
            panel.CreateControl();
            panel.Dock = DockStyle.Fill;
            panel.BackColor = System.Drawing.Color.Black;
            panel.BorderStyle = BorderStyle.None;
            panel.Margin = new System.Windows.Forms.Padding(0);
            FormHost.Child = panel;
        }

        private void FormHost_SizeChanged(object sender, SizeChangedEventArgs e)
        {
        }

        private void ClearButton_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.MessageBoxButton buttons = System.Windows.MessageBoxButton.YesNo;
            var res = System.Windows.MessageBox.Show("This action will delete everything\nrelated to the current scene. Continue?", "Are you sure?", buttons, System.Windows.MessageBoxImage.Question);
            if (res == MessageBoxResult.Yes)
            {
                SendMessage(pOwner, 0x1201, IntPtr.Zero, IntPtr.Zero);
            }
        }

        private void BrowserButton_Click(object sender, RoutedEventArgs e)
        {
            SendMessage(pOwner, 0x1203, IntPtr.Zero, IntPtr.Zero);

            GC.Collect();
        }

        internal void PushIntoLogger(string input)
        {
            OutputConsole.Text += input;
            OutputConsole.ScrollToEnd();
        }

        public void ParentRender(IntPtr child)
        {
            child_hwnd = child;
            SetParent(child, panel.Handle);
            UpdateChildPosition();
        }

        public void UpdateChildPosition()
        {
            if (child_hwnd != null)
            {
                SetWindowPos(child_hwnd, IntPtr.Zero, -3, -3, panel.Width + 6, panel.Height + 6, 0);
            }
        }

        internal void UpdateFrameCounter(double frames)
        {
            FrameBlock.Text = "FPS : " + frames.ToString("0.0");
        }

        private void EntityButton_Click(object sender, RoutedEventArgs e)
        {

        }
    }
}
