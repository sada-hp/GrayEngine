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
        ColorPicker picker;
        public event DummyEvent ColorPropertyChanged;
        int red;
        int green;
        int blue;
        int alpha;

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
            picker = new ColorPicker();
            MainGrid.Children.Add(picker);
            picker.SelectedColorChanged += ColorUpdate;
            picker.ShowAvailableColors = false;
            picker.UsingAlphaChannel = true;
        }
        public void Refresh()
        {
            Delegate d = Delegate.CreateDelegate(Type.GetType("void"), this, "Test");
            this.Dispatcher.Invoke(System.Windows.Threading.DispatcherPriority.Render, d);
        }

        public void Init(string content)
        {
            prop_content = content;

            var color = prop_content.Split(':');
            float r = Convert.ToInt32(float.Parse(color[0], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) * 255);
            float g = Convert.ToInt32(float.Parse(color[1], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) * 255);
            float b = Convert.ToInt32(float.Parse(color[2], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) * 255);
            float a = Convert.ToInt32(float.Parse(color[3], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) * 255);
            picker.SelectedColor = System.Windows.Media.Color.FromArgb((byte)a, (byte)r, (byte)g, (byte)b);
        }

        public void ChangeColors(System.Windows.Media.Brush background, System.Windows.Media.Brush foreground)
        {
            MainGrid.Background = background;
        }

        private void ColorUpdate(object sender, RoutedPropertyChangedEventArgs<System.Windows.Media.Color?> e)
        {
            red = (int)e.NewValue.Value.R;
            green = (int)e.NewValue.Value.G;
            blue = (int)e.NewValue.Value.B;
            alpha = (int)e.NewValue.Value.A;

            Contents = red.ToString() + ':' + green.ToString() + ':' + blue.ToString() + ':' + alpha.ToString();
            ColorPropertyChanged.Invoke(this);
        }

        public int Red()
        {
            return red;
        }

        public int Green()
        {
            return green;
        }

        public int Blue()
        {
            return blue;
        }

        public int Alpha()
        {
            return alpha;
        }

        private void Test(object sender)
        {
        }
    }
}
