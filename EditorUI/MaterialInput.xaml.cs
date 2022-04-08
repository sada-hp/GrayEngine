using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using System.Windows.Controls;
using System.Windows.Forms;

namespace EditorUI
{
    public partial class MaterialInput : System.Windows.Controls.UserControl
    {
        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hWnd, int wMsg, IntPtr wParam, IntPtr lParam);

        IntPtr pOwner;
        public int material_index;
        public MaterialInput(IntPtr p)
        {
            pOwner = p;
            InitializeComponent();
        }

        private void LoadBtn_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "png files (*.png)|*.png";
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                SendMessage(pOwner, 0x1202, (IntPtr)material_index, Marshal.StringToHGlobalAnsi(openFileDialog.FileName));
            }

            GC.Collect();
        }
    }
}
