using System;
using System.Reflection;
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
        void Init(string content);
        void ChangeColors(System.Windows.Media.Brush background, System.Windows.Media.Brush foreground);
    }

    public partial class MainView : Window, EditorWindow
    {
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AddEntity();
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void GetEntityInfo(IntPtr ID);

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void GetEntitiesList();

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LogMessage(IntPtr msg);

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void InitModelBrowser();

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void UpdateEntityProperty(int ID, IntPtr property, IntPtr value);

        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void UpdateSkybox(IntPtr East, IntPtr West, IntPtr Top, IntPtr Bottom, IntPtr North, IntPtr South);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void CloseContext();
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SaveScreenshot(IntPtr filepath);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void TogglePhysics();
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AddNewEntityProperty(int ID, IntPtr property);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SaveScene(IntPtr path);
        [DllImport("SceneEditor.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void LoadScene(IntPtr path);
        [DllImport("user32.dll")]
        static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);
        [DllImport("user32.dll")]
        static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        public Form viewport = new Form();
        IntPtr child_hwnd;
        System.Windows.Forms.Panel panel = new System.Windows.Forms.Panel();
        ObservableCollection<object> entities = new ObservableCollection<object>();
        ObservableCollection<PropertyItem> ent_props = new ObservableCollection<PropertyItem>();
        System.Windows.Controls.ListBoxItem SelectedEntity = new System.Windows.Controls.ListBoxItem();

        public MainView()
        {
            InitializeComponent();
            panel.CreateControl();
            panel.Dock = DockStyle.Fill;
            panel.BackColor = System.Drawing.Color.Black;
            panel.BorderStyle = BorderStyle.None;
            panel.Margin = new System.Windows.Forms.Padding(0);
            FormHost.Child = panel;
            EntitiesList.ItemsSource = entities;
            PropertiesCollection.ItemsSource = ent_props;
        }

        private void FormHost_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateChildPosition();
        }

        private void ClearButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog dlg = new SaveFileDialog();
            dlg.DefaultExt = ".ppm";
            dlg.Filter = "ppm files(*.ppm) | *.ppm";
            DialogResult result = dlg.ShowDialog();

            if (result == System.Windows.Forms.DialogResult.OK && !string.IsNullOrWhiteSpace(dlg.FileName))
            {
                SaveScreenshot(Marshal.StringToHGlobalAnsi(dlg.FileName));
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
            AddEntity();
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

        internal void UpdateProperties(int id, Dictionary<string, object> properties, Dictionary<string, Type> types, Dictionary<string, string> events, Dictionary<string, string> handlers)
        {
            foreach (var pair in properties)
            {
                PropertyItem item = ent_props.Where(x => x.PropertyName == pair.Key).FirstOrDefault();
                var control = Activator.CreateInstance(types[pair.Key]);

                if (item == null)
                {
                    item = new PropertyItem();
                    item.PropertyName = pair.Key;
                    item.ID = id;
                    item.HorizontalAlignment = System.Windows.HorizontalAlignment.Stretch;
                    item.Width = PropertiesCollection.ActualWidth;

                    ((PropertyControl)control).Init(pair.Value.ToString());
                    ((PropertyControl)control).ChangeColors(new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(36, 36, 39)), new System.Windows.Media.SolidColorBrush(System.Windows.Media.Colors.WhiteSmoke));
                    ((PropertyControl)control).ID = id;
                    ((System.Windows.Controls.Control)control).HorizontalAlignment = System.Windows.HorizontalAlignment.Stretch;
                    System.Windows.Controls.Grid.SetColumn((System.Windows.Controls.Control)control, 1);

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
                            LogMessage(Marshal.StringToHGlobalAnsi(e.Message));
                            LogMessage(Marshal.StringToHGlobalAnsi(pair.Key));
                        }
                    }

                    ent_props.Add(item);
                    item.PropGrid.Children.Add((System.Windows.Controls.Control)control);
                }
                else
                {
                    (item.PropGrid.Children[item.PropGrid.Children.Count - 1] as PropertyControl).Contents = pair.Value.ToString();
                }

            }
        }

        private void Drawable_callback(object sender)
        {
            InitModelBrowser();
        }

        private void EntityPosition_callback(object sender)
        {
            UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("EntityPosition"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void EntityOrientation_callback(object sender)
        {
            UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("EntityOrientation"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void Shader_callback(object sender)
        {

        }

        private void EntityName_callback(object sender)
        {
            try
            {
                UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("EntityName"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
                UpdateEntity(((PropertyControl)sender).ID, ((PropertyControl)sender).Contents);
            }
            catch (Exception e)
            {
                LogMessage(Marshal.StringToHGlobalAnsi(e.Message));
            }
        }

        private void Scale_callback(object sender)
        {
            UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("Scale"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void Color_callback(object sender)
        {
            var color = ((PropertyControl)sender).Contents.Split(':');
            float r = float.Parse(color[0], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) / 255;
            float g = float.Parse(color[1], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) / 255;
            float b = float.Parse(color[2], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) / 255;
            float a = float.Parse(color[3], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) / 255;
            UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("Color"), Marshal.StringToHGlobalAnsi(r.ToString().Replace(',','.') + ":" + g.ToString().Replace(',', '.') + ":" + b.ToString().Replace(',', '.') + ":" + a.ToString().Replace(',', '.')));
        }

        private void Mass_callback(object sender)
        {
            UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("Mass"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        internal void RetrieveEntityInfo(int id, string name, string value, string type)
        {
            try
            {
                Dictionary<string, object> properties = new Dictionary<string, object>();
                Dictionary<string, Type> types = new Dictionary<string, Type>();
                Dictionary<string, string> events = new Dictionary<string, string>();
                Dictionary<string, string> handlers = new Dictionary<string, string>();

                properties.Add(name, value);

                if (type == "vector3" || type == "quat")
                {
                    types.Add(name, typeof(_3VectorControl));
                    events.Add(name, "VectorPropertyChanged");
                }
                else if (name == "EntityID")
                {
                    types.Add(name, typeof(TextControl));
                }
                else if (name == "Color")
                {
                    types.Add(name, typeof(ColorControl));
                    events.Add(name, "ColorPropertyChanged");
                }
                else if (name == "Drawable")
                {
                    types.Add(name, typeof(ResourceControl));
                    events.Add(name, "DialogOpen");
                }
                else
                {
                    types.Add(name, typeof(LabelControl));
                    events.Add(name, "TextBoxTextChanged");
                }

                handlers.Add(name, name + "_callback");

                UpdateProperties(id, properties, types, events, handlers);
            }
            catch (Exception e)
            {
                LogMessage(Marshal.StringToHGlobalAnsi(e.Message + e.StackTrace));
            }
        }

        private void EntitiesList_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            try
            {
                if (e.AddedItems.Count > 0)
                {
                    ent_props.Clear();
                    GetEntityInfo((IntPtr)(e.AddedItems[0] as EntityItem).ID);
                }
            }
            catch (Exception ee)
            {
                LogMessage(Marshal.StringToHGlobalAnsi(ee.Message));
            }
        }

        internal void SelectEntity(int ID)
        {
            if (ID == 0)
            {
                EntitiesList.SelectedItem = null;
                ent_props.Clear();
            }
            
            for (int i = 0; i < EntitiesList.Items.Count; i++)
            {
                if ((EntitiesList.Items.GetItemAt(i) as EntityItem).ID == ID)
                {
                    EntitiesList.SelectedItem = EntitiesList.Items.GetItemAt(i);
                    return;
                }
            }
        }

        internal void RemoveEntity(int ID)
        {
            try
            {
                for (int i = 0; i < entities.Count; i++)
                {
                    if ((entities[i] as EntityItem).ID == ID)
                    {
                        ent_props.Clear();
                        entities.RemoveAt(i);
                        return;
                    }
                }
            }
            catch (Exception ee)
            {
                LogMessage(Marshal.StringToHGlobalAnsi(ee.Message));
            }
        }

        private void SkyboxSettings_Click(object sender, RoutedEventArgs e)
        {
            SkyboxSettings settings = new SkyboxSettings();
            settings.ShowDialog();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            CloseContext();
        }

        private void PhysicsButton_Click(object sender, RoutedEventArgs e)
        {
            TogglePhysics();
        }

        private void ExtraProps_PreviewMouseUp(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            ExtraProps.IsSubmenuOpen = !ExtraProps.IsSubmenuOpen;
        }

        private void MenuItem_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                AddNewEntityProperty((EntitiesList.SelectedItem as EntityItem).ID, Marshal.StringToHGlobalAnsi((sender as System.Windows.Controls.Control).Name));
                ent_props.Clear();
                GetEntityInfo((IntPtr)((EntitiesList.SelectedItem as EntityItem).ID));
                ExtraProps.IsSubmenuOpen = false;
            }
            catch (Exception ee)
            {
                LogMessage(Marshal.StringToHGlobalAnsi(ee.Message));
            }
        }

        private void DockPanel_LostFocus(object sender, RoutedEventArgs e)
        {

        }

        private void SaveBtn_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog dlg = new SaveFileDialog();
            dlg.RestoreDirectory = true;
            dlg.Filter = "GRengine level file (*.glf)|*.glf"; ;

            if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                SaveScene(Marshal.StringToHGlobalAnsi(dlg.FileName));
            }

            GC.Collect();
        }

        private void LoadBtn_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog dlg = new OpenFileDialog();
            dlg.RestoreDirectory = true;
            dlg.Filter = "GRengine level file (*.glf)|*.glf"; ;

            if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                LoadScene(Marshal.StringToHGlobalAnsi(dlg.FileName));
                ent_props.Clear();
                entities.Clear();
                GetEntitiesList();
            }

            GC.Collect();
        }
    };
}