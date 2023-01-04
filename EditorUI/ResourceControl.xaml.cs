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
    public partial class ResourceControl : UserControl, PropertyControl
    {
        public event DummyEvent ValueChanged;
        public event DummyEvent DialogOpen;
        string content_folder = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + @"\Content\";
        int entity_id;
        string prop_content;
        public string Contents
        {
            get => prop_content;
            set
            {
                prop_content = value;
                Src.Text = value;
            }
        }

        public int ID
        {
            get => entity_id;
            set
            {
                entity_id = value;
            }
        }

        public ResourceControl()
        {
            InitializeComponent();
            ValueChanged += Test;
            DialogOpen += Test;
        }

        public void Init(string value)
        {
            Contents = value;
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            DialogOpen.Invoke(this);

            //System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            //openFileDialog.Filter = "image files (*.png)|*.png";
            //openFileDialog.RestoreDirectory = true;

            //if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            //{
            //    Contents = openFileDialog.FileName;
            //    Src.ScrollToEnd();
            //    ValueChanged.Invoke(this);
            //}

            //GC.Collect();
        }

        public void ChangeColors(System.Windows.Media.Brush background, System.Windows.Media.Brush foreground)
        {
            Src.Background = background;
            Src.Foreground = foreground;
        }

        private void Test(object sender)
        {
        }
    }
}
