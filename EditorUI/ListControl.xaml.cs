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
        bool silent = false;
        string prop_content = "";
        int entity_id;
        public event DummyEvent ControlSelectionChanged;

        public string Contents
        {
            get => Container.SelectedItem == null ? "" : ((ComboBoxItem)Container.SelectedItem).Content.ToString();
            set
            {
                Container.Items.Clear();
                var content = value.Split(':');
                prop_content = value;

                for (int i = 0; i < content.Length; i++)
                {
                    ComboBoxItem item = new ComboBoxItem();
                    item.Content = content[i];
                    Container.Items.Add(item);
                }

                silent = true;
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
            bool f = false;
            foreach (ComboBoxItem o in Container.Items)
            {
                if (o.IsFocused)
                {
                    f = true;
                    break;
                }
            }
            if (Container.IsFocused || f)
                ControlSelectionChanged(this);

            silent = false;
        }
    }
}
