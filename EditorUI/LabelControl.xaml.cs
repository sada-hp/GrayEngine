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
using System.Threading;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;


namespace EditorUI
{
    /// <summary>
    /// Логика взаимодействия для LabelControl.xaml
    /// </summary>
    public partial class LabelControl : UserControl, PropertyControl
    {
        string prop_content = "";

        public string Contents
        {
            get => prop_content;
            set
            {
                prop_content = value;
                ContentLabel.Content = prop_content;
            }
        }

        public void ChangeColors(System.Windows.Media.Brush background, System.Windows.Media.Brush foreground)
        {
            ContentLabel.Background = background;
            ContentLabel.Foreground = foreground;
        }

        public LabelControl()
        {
            InitializeComponent();
        }
    }
}
