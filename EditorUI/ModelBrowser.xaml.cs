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
    public partial class ModelBrowser : Window, EditorWindow
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
        IntPtr child_hwnd;
        System.Windows.Forms.Panel panel = new System.Windows.Forms.Panel();

        public ModelBrowser(IntPtr p)
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
                SetWindowPos(child_hwnd, IntPtr.Zero, 0, 0, panel.Width, panel.Height, 0);
            }
        }

        private void MdlBrowser_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
        }

        private void BtnLoadMdl_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "obj files (*.obj)|*.obj";
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                SendMessage(pOwner, 0x1200, IntPtr.Zero, Marshal.StringToHGlobalAnsi(openFileDialog.FileName));
            }

            GC.Collect();
        }

        private void MdlBrowser_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateChildPosition();
        }

        internal void AddMaterialToTheTable(string string_names)
        {
            MaterialsPanel.Children.Clear();

            int mat_index = 0;
            string[] material_names = string_names.Split('\\');
            
            foreach (string material_name in material_names)
            {
                if (material_name != String.Empty)
                {
                    MaterialInput material_panel = new MaterialInput(pOwner);

                    material_panel.NameTextBlock.Text = material_name;
                    System.Windows.Controls.DockPanel.SetDock(material_panel, System.Windows.Controls.Dock.Top);
                    material_panel.material_index = mat_index++;

                    MaterialsPanel.Children.Add(material_panel);
                }
            }
        }
    }
}
