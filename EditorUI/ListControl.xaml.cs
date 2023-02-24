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
using System.ComponentModel;

namespace EditorUI
{
    /// <summary>
    /// Логика взаимодействия для ListControl.xaml
    /// </summary>
    public partial class ListControl : UserControl, PropertyControl
    {
        string prop_content = "";
        int entity_id;
        public event DummyEvent ControlSelectionChanged;

        public string Contents
        {
            get => Container.SelectedItem.ToString();
            set
            {
                Container.Items.Clear();
                var content = value.Split(':');
                prop_content = value;

                for (int i = 0; i < content.Length; i++)
                {
                    Container.Items.Add(content[i]);
                }
                Container.SelectedIndex = 0;
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
        public ListControl()
        {
            InitializeComponent();
            ControlSelectionChanged += Test;
        }

        public void ChangeColors(System.Windows.Media.Brush background, System.Windows.Media.Brush foreground)
        {
            Container.Resources.Add(SystemColors.WindowBrushKey, background);
            Container.Background = background;
            Container.Foreground = foreground;
        }

        private void Test(object sender)
        {

        }

        private void Container_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ControlSelectionChanged(this);
        }
    }
}
