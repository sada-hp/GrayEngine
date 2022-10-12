using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using System.Windows.Forms;
using System.Threading;

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
            }
        }

        private void BrowserButton_Click(object sender, RoutedEventArgs e)
        {
            SendMessage(pOwner, 0x1203, IntPtr.Zero, IntPtr.Zero);

            GC.Collect();
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
            SendMessage(pOwner, 0x1204, IntPtr.Zero, IntPtr.Zero);
        }

        internal void UpdateEntity(int ID, string name)
        {
            EntityItem button = new EntityItem(ID);
            button.Content = name;
            button.Background = null;
            button.Foreground = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Colors.WhiteSmoke);
            EntitiesList.Items.Add(button);
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
                        SendMessage(pOwner, 0x1119, Marshal.StringToHGlobalAnsi(e.Message), IntPtr.Zero);
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
        }

        private void UpdateObjectPosition(object sender)
        {
            var coords = (sender as _3VectorControl).Contents.Split(':');
            SendMessage(pOwner, 0x1206, Marshal.StringToHGlobalAnsi("posx"), Marshal.StringToHGlobalAnsi(coords[0]));
        }

        internal void RetrieveEntityInfo(int ID, string name, float posx, float posy, float posz)
        {
            Dictionary<string, object> properties = new Dictionary<string, object>();
            Dictionary<string, Type> types = new Dictionary<string, Type>();
            Dictionary<string, string> events = new Dictionary<string, string>();
            Dictionary<string, string> handlers = new Dictionary<string, string>();

            properties.Add("EntityName", name);
            properties.Add("EntityID", ID);
            properties.Add("Drawable", "None");
            properties.Add("Position", posx.ToString() + ":" + posy.ToString() + ":" + posz.ToString());
            types.Add("EntityName", typeof(LabelControl));
            types.Add("EntityID", typeof(LabelControl));
            types.Add("Drawable", typeof(LabelControl));
            types.Add("Position", typeof(_3VectorControl));
            events.Add("Drawable", "MouseDoubleClick");
            events.Add("Position", "VectorPropertyChanged");
            handlers.Add("Drawable", "LoadModelBrowser");
            handlers.Add("Position", "UpdateObjectPosition");

            UpdateProperties(properties, types, events, handlers);
        }

        private void EntitiesList_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            SendMessage(pOwner, 0x1205, (IntPtr)(e.AddedItems[0] as EntityItem).ID, IntPtr.Zero);
        }
    };
}