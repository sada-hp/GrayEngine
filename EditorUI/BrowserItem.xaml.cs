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

namespace EditorUI
{
    /// <summary>
    /// Логика взаимодействия для BrowserItem.xaml
    /// </summary>
    public partial class BrowserItem : UserControl
    {
        public string mesh_path = "";
        public string collision_path = "";
        public BrowserItem(string mesh_path)
        {
            InitializeComponent();
            Mesh_Label.Content = mesh_path;
        }
    }
}
