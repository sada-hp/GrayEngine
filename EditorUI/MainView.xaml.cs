using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using System.Windows.Forms;

namespace EditorUI
{
    /// <summary>
    /// Логика взаимодействия для MainView.xaml
    /// </summary>
    public partial class MainView : Window
    {
        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hWnd, int wMsg, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")]
        public static extern int FindWindow(string lpClassName, String lpWindowName);
        [DllImport("user32.dll")]
        static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);
        [DllImport("user32.dll")]
        static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        IntPtr p_Parent;
        public Form viewport = new Form();
        IntPtr child_hwnd;
        System.Windows.Forms.Panel panel = new System.Windows.Forms.Panel();

        public MainView(IntPtr p)
        {
            p_Parent = p;
            InitializeComponent();
            panel.CreateControl();
            panel.Dock = DockStyle.Fill;
            panel.BackColor = System.Drawing.Color.Black;
            panel.BorderStyle = BorderStyle.None;
            panel.Margin = new System.Windows.Forms.Padding(0);
            FormHost.Child = panel;
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "obj files (*.obj)|*.obj";
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                SendMessage(p_Parent, 0x1200, IntPtr.Zero, Marshal.StringToHGlobalAnsi(openFileDialog.FileName));
            }

            GC.Collect();
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
                SendMessage(p_Parent, 0x1201, IntPtr.Zero, IntPtr.Zero);
            }
        }

        internal void PushIntoLogger(string input)
        {
            OutputConsole.Text += input;
            OutputConsole.ScrollToEnd();
        }

        internal void ParentRender(IntPtr child)
        {
            child_hwnd = child;
            SetParent(child, panel.Handle);
            UpdateChildPosition();
        }

        internal void UpdateChildPosition()
        {
            if (child_hwnd != null)
            {
                SetWindowPos(child_hwnd, IntPtr.Zero, -3, -3, panel.Width + 6, panel.Height + 6, 0);
            }
        }
    }
}
