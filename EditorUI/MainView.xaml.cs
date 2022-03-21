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

        IntPtr p_Parent;
        IntPtr p_Viewport;
        public Form viewport = new Form();

        public MainView(IntPtr p)
        {
            p_Parent = p;
            InitializeComponent();
            viewport.TopLevel = false;
            FormHost.Child = viewport;
            viewport.FormBorderStyle = FormBorderStyle.FixedSingle;
            viewport.Show();
            FormHost.BringIntoView();
            viewport.AutoSize = true;
            viewport.BackColor = System.Drawing.Color.Black;

            //OutputConsole.Document.Blocks.Clear();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            //openFileDialog.InitialDirectory = "c:\\";
            openFileDialog.Filter = "obj files (*.obj)|*.obj";
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                SendMessage(p_Parent, 0x1200, IntPtr.Zero, Marshal.StringToHGlobalAnsi(openFileDialog.FileName));
            }
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
            GC.Collect();
        }
    }
}
