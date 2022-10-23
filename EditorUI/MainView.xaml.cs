using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using System.Windows.Forms;
using System.Threading;
using System.Collections.ObjectModel;

namespace EditorUI
{
    struct EntityProps
    {
        public uint ID;
        public string Name;

        public EntityProps(uint id, string name)
        {
            ID = id;
            Name = name;
        }
    };
    
    interface PropertyControl
    {
        string Contents { get; set; }
        int ID { get; set; }
        void ChangeColors(System.Windows.Media.Brush background, System.Windows.Media.Brush foreground);
    }

    public partial class MainView : Window, EditorWindow
    {
        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hWnd, int wMsg, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")]
        public static extern int FindWindow(string lpClassName, String lpWindowName);
        [DllImport("user32.dll")]
        static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);
        [DllImport("user32.dll")]
        static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        IntPtr pOwner;
        public Form viewport = new Form();
        IntPtr child_hwnd;
        System.Windows.Forms.Panel panel = new System.Windows.Forms.Panel();
        ObservableCollection<object> entities = new ObservableCollection<object>();

        public MainView()
        {
            InitializeComponent();
            panel.CreateControl();
            panel.Dock = DockStyle.Fill;
            panel.BackColor = System.Drawing.Color.Black;
            panel.BorderStyle = BorderStyle.None;
            panel.Margin = new System.Windows.Forms.Padding(0);
            FormHost.Child = panel;
        }

        public MainView(IntPtr p)
        {
            pOwner = p;
            InitializeComponent();
            panel.CreateControl();
            panel.Dock = DockStyle.Fill;
            panel.BackColor = System.Drawing.Color.Black;
            panel.BorderStyle = BorderStyle.None;
            panel.Margin = new System.Windows.Forms.Padding(0);
            FormHost.Child = panel;
            EntitiesList.ItemsSource = entities;
        }

        private void FormHost_SizeChanged(object sender, SizeChangedEventArgs e)
        {
        }

        private void ClearButton_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.MessageBoxButton buttons = System.Windows.MessageBoxButton.YesNo;
            var res = System.Windows.MessageBox.Show("This action will delete everything\nrelated to the current scene. Continue?", "Are you sure?", buttons, System.Windows.MessageBoxImage.Question);
            if (res == MessageBoxResult.Yes)
            {
                SendMessage(pOwner, 0x1201, IntPtr.Zero, IntPtr.Zero);
                entities.Clear();
            }
        }

        internal void PushIntoLogger(string input)
        {
            OutputConsole.Text += input;
            OutputConsole.ScrollToEnd();
        }

        public void ParentRender(IntPtr child)
        {
            child_hwnd = child;
            SetParent(child, panel.Handle);
            UpdateChildPosition();
        }

        public void UpdateChildPosition()
        {
            if (child_hwnd != null)
            {
                SetWindowPos(child_hwnd, IntPtr.Zero, -3, -3, panel.Width + 6, panel.Height + 6, 0);
            }
        }

        internal void UpdateFrameCounter(double frames)
        {
            FrameBlock.Text = "FPS : " + frames.ToString("0.0");
        }

        private void EntityButton_Click(object sender, RoutedEventArgs e)
        {
            UIBridge.AddEntity();
        }

        internal void UpdateEntity(int ID, string name)
        {
            var item = entities.Where(x => (x as EntityItem).ID == ID);
            if (item.Count() == 0)
            {
                EntityItem button = new EntityItem(ID);
                button.Content = name;
                button.Background = null;
                button.Foreground = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Colors.WhiteSmoke);
                entities.Add(button);
            }
            else
            {
                (item.First() as EntityItem).Content = name;
            }
        }

        internal void UpdateProperties(Dictionary<string, object> properties, Dictionary<string, Type> types, Dictionary<string, string> events, Dictionary<string, string> handlers)
        {
            PropertiesCollection.Items.Clear();

            foreach (var pair in properties)
            {
                PropertyItem item = new PropertyItem();
                var control = Activator.CreateInstance(types[pair.Key]);
                item.PName.Content = pair.Key;
                ((PropertyControl)control).Contents = pair.Value.ToString();

                ((PropertyControl)control).ChangeColors(null, new System.Windows.Media.SolidColorBrush(System.Windows.Media.Colors.WhiteSmoke));
                ((PropertyControl)control).ID = (int)properties["EntityID"];
                ((System.Windows.Controls.Control)control).HorizontalAlignment = System.Windows.HorizontalAlignment.Stretch;

                if (events.ContainsKey(pair.Key))
                {
                    var method = events[pair.Key];

                    var event_type = control.GetType().GetEvent(method);
                    try
                    {
                        Delegate d = Delegate.CreateDelegate(event_type.EventHandlerType, this, handlers[pair.Key]);
                        event_type.AddEventHandler(control, d);

                    }
                    catch (Exception e)
                    {
                        UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(e.Message));
                    }
                }
                System.Windows.Controls.Grid.SetColumn((System.Windows.Controls.Control)control, 1);
                item.PropGrid.Children.Add((System.Windows.Controls.Control)control);
                item.HorizontalAlignment = System.Windows.HorizontalAlignment.Stretch;
                item.Width = PropertiesCollection.ActualWidth;
                PropertiesCollection.Items.Add(item);
            }
        }

        public void LoadModelBrowser(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            SendMessage(pOwner, 0x1203, IntPtr.Zero, IntPtr.Zero);
            //UIBridge.InitModelBrowser();
        }

        private void UpdateObjectPosition(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("position"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void UpdateObjectOrientation(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("orientation"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void UpdateObjectName(object sender)
        {
            try
            {
                UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("name"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
                UpdateEntity(((PropertyControl)sender).ID, ((PropertyControl)sender).Contents);
            }
            catch (Exception e)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(e.Message));
            }
        }

        internal void RetrieveEntityInfo(int ID, string name, float posx, float posy, float posz)
        {
            try
            {
                Dictionary<string, object> properties = new Dictionary<string, object>();
                Dictionary<string, Type> types = new Dictionary<string, Type>();
                Dictionary<string, string> events = new Dictionary<string, string>();
                Dictionary<string, string> handlers = new Dictionary<string, string>();

                properties.Add("EntityName", name);
                properties.Add("EntityID", ID);
                properties.Add("Drawable", "None");
                properties.Add("Position", posx.ToString() + ":" + posy.ToString() + ":" + posz.ToString());
                properties.Add("Orientation", posx.ToString() + ":" + posy.ToString() + ":" + posz.ToString());
                types.Add("EntityName", typeof(LabelControl));
                types.Add("EntityID", typeof(TextControl));
                types.Add("Drawable", typeof(TextControl));
                types.Add("Position", typeof(_3VectorControl));
                types.Add("Orientation", typeof(_3VectorControl));
                events.Add("EntityName", "TextBoxTextChanged");
                events.Add("Drawable", "MouseDoubleClick");
                events.Add("Position", "VectorPropertyChanged");
                events.Add("Orientation", "VectorPropertyChanged");
                handlers.Add("EntityName", "UpdateObjectName");
                handlers.Add("Drawable", "LoadModelBrowser");
                handlers.Add("Position", "UpdateObjectPosition");
                handlers.Add("Orientation", "UpdateObjectOrientation");

                UpdateProperties(properties, types, events, handlers);
            }
            catch (Exception e)
            {
                SendMessage(pOwner, 0x1119, Marshal.StringToHGlobalAnsi(e.Message), IntPtr.Zero);
            }
        }

        private void EntitiesList_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            try
            {
                if (e.AddedItems.Count > 0)
                    SendMessage(pOwner, 0x1205, (IntPtr)(e.AddedItems[0] as EntityItem).ID, IntPtr.Zero);
            }
            catch (Exception ee)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(ee.Message));
            }
        }
    };
}