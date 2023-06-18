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
    /// Логика взаимодействия для EntityItem.xaml
    /// </summary>
    public partial class EntityItem : UserControl
    {
        int EntityID;
        public int ID
        {
            get => EntityID;
        }

        public string EntityName
        {
            get => Content.ToString();
        }

        public EntityItem(int id)
        {
            EntityID = id;
            InitializeComponent();
        }
    }
}
