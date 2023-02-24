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
using System.ComponentModel;

namespace EditorUI
{
    public delegate void DummyEvent(object sender);
    public partial class _3VectorControl : UserControl, PropertyControl
    {
        string prop_content = "";
        int entity_id;
        public event DummyEvent VectorPropertyChanged;

        public string Contents
        {
            get => prop_content;
            set
            {
                prop_content = value;
                var coords = prop_content.Split(':');
                if (coords.Length < 3) return;

                XBox.Text = coords[0];
                YBox.Text = coords[1];
                ZBox.Text = coords[2];
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
            XBox.Background = background;
            XBox.Foreground = foreground;
            YBox.Background = background;
            YBox.Foreground = foreground;
            ZBox.Background = background;
            ZBox.Foreground = foreground;
        }

        public _3VectorControl()
        {
            InitializeComponent();
            VectorPropertyChanged += Test;
            DataContext = this;
        }

        private void UserControl_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {

        }

        private void TextInputHandler(object sender, TextChangedEventArgs e)
        {
            if (XBox.IsKeyboardFocused || YBox.IsKeyboardFocused || ZBox.IsKeyboardFocused)
            {
                Contents = XBox.Text + ':' + YBox.Text + ':' + ZBox.Text;
                VectorPropertyChanged.Invoke(this);
            }
        }

        private void Test(object sender)
        {
        }
    }
}
