using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
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
    /// Логика взаимодействия для PropertyItem.xaml
    /// </summary>
    public partial class PropertyItem : UserControl
    {
        private int id;
        private string name;

        public int ID
        {
            get => id;
            set
            {
                id = value;
            }
        }

        public string PropertyName
        {
            get => name;
            set
            {
                name = value;
                PName.Content = value;
            }
        }
        public PropertyItem()
        {
            InitializeComponent();
        }
    }
}
