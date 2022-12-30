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
using Xceed.Wpf.Toolkit;

namespace EditorUI
{
    /// <summary>
    /// Логика взаимодействия для ColorControl.xaml
    /// </summary>
    public partial class ColorControl : UserControl, PropertyControl
    {
        string prop_content = "";
        int entity_id;
        public event DummyEvent ColorPropertyChanged;

        public string Contents
        {
            get => prop_content;
            set
            {
                prop_content = value;
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

        public ColorControl()
        {
            InitializeComponent();
            ColorPropertyChanged += Test;
            ColorPicker picker = new ColorPicker();
            MainGrid.Children.Add(picker);
            picker.SelectedColorChanged += ColorUpdate;
            picker.ShowAvailableColors = false;
            picker.UsingAlphaChannel = true;
        }

        public void ChangeColors(System.Windows.Media.Brush background, System.Windows.Media.Brush foreground)
        {
            MainGrid.Background = background;
        }

        private void ColorUpdate(object sender, RoutedPropertyChangedEventArgs<System.Windows.Media.Color?> e)
        {

            int red = (int)e.NewValue.Value.R;
            int green = (int)e.NewValue.Value.G;
            int blue = (int)e.NewValue.Value.B;
            int alpha = (int)e.NewValue.Value.A;

            Contents = red.ToString() + ':' + green.ToString() + ':' + blue.ToString() + ':' + alpha.ToString();
            ColorPropertyChanged.Invoke(this);
        }

        private void Test(object sender)
        {
        }
    }
}
