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
    public partial class CheckBoxControl : UserControl, PropertyControl
    {
        string prop_content = "";
        int entity_id;
        public event DummyEvent CheckPropertyChanged;

        public string Contents
        {
            get => prop_content;
            set
            {
                if (value == "0")
                {
                    CBB.IsChecked = false;
                }
                else
                {
                    CBB.IsChecked = true;
                }
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
            CBB.Background = background;
            CBB.Foreground = foreground;
        }

        public CheckBoxControl()
        {
            InitializeComponent();
        }

        private void CBB_Click(object sender, RoutedEventArgs e)
        {
            prop_content = (bool)CBB.IsChecked ? "1" : "0";
            CheckPropertyChanged.Invoke(this);
        }
    }
}
