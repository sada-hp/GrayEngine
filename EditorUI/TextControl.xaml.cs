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
    /// Логика взаимодействия для TextElement.xaml
    /// </summary>
    public partial class TextControl : UserControl, PropertyControl
    {
        string prop_content = "";
        int entity_id;

        public string Contents
        {
            get => prop_content;
            set
            {
                prop_content = value;
                ContentLabel.Content = prop_content;
                ContentLabel.ToolTip = prop_content;
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

        public void Init(string content)
        {
            Contents = content;
        }

        public void ChangeColors(System.Windows.Media.Brush background, System.Windows.Media.Brush foreground)
        {
            ContentLabel.Background = background;
            ContentLabel.Foreground = foreground;
        }
        public TextControl()
        {
            InitializeComponent();
        }
    }
}
