using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Runtime.InteropServices;


namespace EditorUI
{
    /// <summary>
    /// Логика взаимодействия для ResourceControl.xaml
    /// </summary>
    public partial class ResourceControl : UserControl
    {
        public event DummyEvent ValueChanged;
        string content_folder = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + @"\Content\";
        public object Value
        {
            get => Src.Text;
        }

        public ResourceControl()
        {
            InitializeComponent();
            ValueChanged += Test;
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                Src.Text = openFileDialog.FileName;
                Src.ScrollToEnd();
                ValueChanged.Invoke(this);
            }

            GC.Collect();
        }

        private void Test(object sender)
        {
        }
    }
}
