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
using System.Windows.Threading;
using System.Collections.ObjectModel;

namespace EditorUI
{
    enum InfoChunks
    {
        FRAMES = 0,
        ENTITY_INFO = 1,
        SELECTION = 2,
        LOG
    }

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
        int selectinon_id;
        int brush_mode = 0;
        public Form viewport = new Form();
        IntPtr child_hwnd;
        System.Windows.Forms.Panel panel = new System.Windows.Forms.Panel();
        ObservableCollection<object> entities = new ObservableCollection<object>();
        ObservableCollection<object> entities_filter = new ObservableCollection<object>();
        ObservableCollection<PropertyItem> ent_props = new ObservableCollection<PropertyItem>();

        public MainView()
        {
            InitializeComponent();
            panel.CreateControl();
            panel.Dock = DockStyle.Fill;
            panel.BackColor = System.Drawing.Color.Black;
            panel.BorderStyle = BorderStyle.None;
            panel.Margin = new System.Windows.Forms.Padding(0);
            FormHost.Child = panel;
            EntitiesList.ItemsSource = entities_filter;
            PropertiesCollection.ItemsSource = ent_props;
        }

        private void FormHost_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateChildPosition();
        }

        private void ClearButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileDialog dlg = new SaveFileDialog();
            dlg.DefaultExt = ".png";
            dlg.Filter = "png files(*.png) | *.png";
            DialogResult result = dlg.ShowDialog();

            if (result == System.Windows.Forms.DialogResult.OK && !string.IsNullOrWhiteSpace(dlg.FileName))
            {
                UIBridge.SaveScreenshot(Marshal.StringToHGlobalAnsi(dlg.FileName));
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
            UIBridge.SetParent(child, panel.Handle);
            UpdateChildPosition();
        }

        public void UpdateChildPosition()
        {
            if (child_hwnd != null)
            {
                UIBridge.SetWindowPos(child_hwnd, IntPtr.Zero, -3, -3, panel.Width + 6, panel.Height + 6, 0);
            }
        }

        public void OpenFormContextMenu()
        {
            FormHost.ContextMenu.IsOpen = true;
        }

        internal void UpdateFrameCounter(string frames)
        {
            FrameBlock.Text = "FPS : " + frames;
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

                string filter = SearchBar.Text.Replace("Search...", "");

                if (button.EntityName.ToLower().Contains(filter) || filter == "")
                {
                    entities_filter.Add(button);
                }
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

                if (item == null)
                {
                    var control = Activator.CreateInstance(types[pair.Key]);
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
                            UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(e.Message));
                            UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(pair.Key));
                        }
                    }

                    ent_props.Add(item);
                    item.PropGrid.Children.Add((System.Windows.Controls.Control)control);
                }
                else
                {
                    (item.PropGrid.Children[item.PropGrid.Children.Count - 1] as PropertyControl).Contents = pair.Value.ToString();
                }

                System.Windows.Input.Keyboard.ClearFocus();
            }
        }

        private void Drawable_callback(object sender)
        {
            UIBridge.InitModelBrowser();
        }

        private void EntityPosition_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("EntityPosition"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void EntityOrientation_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("EntityOrientation"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void Empty_callback(object sender)
        {

        }

        private void EntityName_callback(object sender)
        {
            try
            {
                UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("EntityName"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
                UpdateEntity(((PropertyControl)sender).ID, ((PropertyControl)sender).Contents);
            }
            catch (Exception e)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(e.Message));
            }
        }

        private void Scale_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("Scale"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void Transparency_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("Transparency"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void CastShadow_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("CastShadow"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void DoubleSided_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("DoubleSided"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void Color_callback(object sender)
        {
            var color = ((PropertyControl)sender).Contents.Split(':');
            float r = float.Parse(color[0], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) / 255;
            float g = float.Parse(color[1], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) / 255;
            float b = float.Parse(color[2], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) / 255;
            float a = float.Parse(color[3], System.Globalization.CultureInfo.InvariantCulture.NumberFormat) / 255;
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("Color"), Marshal.StringToHGlobalAnsi(r.ToString().Replace(',','.') + ":" + g.ToString().Replace(',', '.') + ":" + b.ToString().Replace(',', '.') + ":" + a.ToString().Replace(',', '.')));
        }

        private void Mass_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("Mass"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void AlphaThreshold_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("AlphaThreshold"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void NormalStrength_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("NormalStrength"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void MaximumDistance_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("MaximumDistance"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void Brightness_callback(object sender)
        {
            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("Brightness"), Marshal.StringToHGlobalAnsi(((PropertyControl)sender).Contents));
        }

        private void Opacity_callback(object sender)
        {
            float opac = float.Parse(((PropertyControl)sender).Contents, System.Globalization.CultureInfo.InvariantCulture.NumberFormat);
            UIBridge.UpdateBrush(-1, opac, -1, -1);
        }

        private void Size_callback(object sender)
        {
            float size = float.Parse(((PropertyControl)sender).Contents, System.Globalization.CultureInfo.InvariantCulture.NumberFormat);
            UIBridge.UpdateBrush(-1 , - 1, size, -1);
        }

        private void Falloff_callback(object sender)
        {
            float fall = float.Parse(((PropertyControl)sender).Contents, System.Globalization.CultureInfo.InvariantCulture.NumberFormat);
            UIBridge.UpdateBrush(-1, -1, -1, fall);
        }

        private void CollisionType_callback(object sender)
        {
            string mode = "0";
            if ((sender as PropertyControl).Contents == "Box")
                mode = "0";
            else if ((sender as PropertyControl).Contents == "Sphere")
                mode = "1";
            else if ((sender as PropertyControl).Contents == "Hull")
                mode = "2";
            else
                mode = "3";

            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("CollisionType"), Marshal.StringToHGlobalAnsi(mode));
        }

        private void BodyType_callback(object sender)
        {
            string mode = "0";
            if ((sender as PropertyControl).Contents == "RigidBody")
                mode = "0";
            else if ((sender as PropertyControl).Contents == "KinematicBody")
                mode = "1";
            else if ((sender as PropertyControl).Contents == "StaticObject")
                mode = "2";

            UIBridge.UpdateEntityProperty(((PropertyControl)sender).ID, Marshal.StringToHGlobalAnsi("BodyType"), Marshal.StringToHGlobalAnsi(mode));
        }

        private void BrushMode_callback(object sender)
        {
            if ((sender as PropertyControl).Contents == "Paint")
                UIBridge.UpdateBrush(1, -1, -1, -1);
            else if ((sender as PropertyControl).Contents == "Erase")
                UIBridge.UpdateBrush(2, -1, -1, -1);
        }

        private void SculptMode_callback(object sender)
        {
            if ((sender as PropertyControl).Contents == "Add")
                UIBridge.UpdateBrush(1, -1, -1, -1);
            else if ((sender as PropertyControl).Contents == "Subtract")
                UIBridge.UpdateBrush(2, -1, -1, -1);
            else if ((sender as PropertyControl).Contents == "Flattern")
                UIBridge.UpdateBrush(3, -1, -1, -1);
            else if ((sender as PropertyControl).Contents == "Smooth")
                UIBridge.UpdateBrush(4, -1, -1, -1);
        }

        private void BrushChannels_callback(object sender)
        {
            if ((sender as PropertyControl).Contents == "Red")
                UIBridge.SetActiveBrushChannels(true, false, false);
            else if ((sender as PropertyControl).Contents == "Green")
                UIBridge.SetActiveBrushChannels(false, true, false);
            else if ((sender as PropertyControl).Contents == "Blue")
                UIBridge.SetActiveBrushChannels(false, false, true);
            else if ((sender as PropertyControl).Contents == "RGB")
                UIBridge.SetActiveBrushChannels(true, true, true);
        }

        internal void UpdateInfo(int type, string lp, string rp)
        {
            switch (type)
            {
                case (int)InfoChunks.FRAMES:
                    UpdateFrameCounter(rp);
                    break;
                case (int)InfoChunks.SELECTION:
                    SelectEntity(Convert.ToInt32(rp));
                    break;
                case (int)InfoChunks.ENTITY_INFO:
                    RetrieveEntityInfo(lp, rp);
                    break;
                case (int)InfoChunks.LOG:
                    PushIntoLogger(rp);
                    break;
            }
        }

        internal void RetrieveEntityInfo(string name, string value)
        {
            try
            {
                Dictionary<string, object> properties = new Dictionary<string, object>();
                Dictionary<string, Type> types = new Dictionary<string, Type>();
                Dictionary<string, string> events = new Dictionary<string, string>();
                Dictionary<string, string> handlers = new Dictionary<string, string>();

                properties.Add(name, value);

                if (name == "EntityPosition" || name == "EntityOrientation" || name == "Scale")
                {
                    types.Add(name, typeof(_3VectorControl));
                    events.Add(name, "VectorPropertyChanged");
                }
                else if (name == "EntityName" || name == "AlphaThreshold" || name == "Mass" || name == "MaximumDistance" || name == "Brightness" || name == "NormalStrength")
                {
                    types.Add(name, typeof(LabelControl));
                    events.Add(name, "TextBoxTextChanged");
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
                else if (name == "Transparency" || name == "DoubleSided" || name == "CastShadow")
                {
                    types.Add(name, typeof(CheckBoxControl));
                    events.Add(name, "CheckPropertyChanged");
                }
                else if (name == "BodyType")
                {
                    if (value == "0")
                    {
                        properties[name] = "RigidBody:KinematicBody:StaticObject";
                    }
                    else if (value == "1")
                    {
                        properties[name] = "KinematicBody:RigidBody:StaticObject";
                    }
                    else if (value == "2")
                    {
                        properties[name] = "StaticObject:RigidBody:KinematicBody";
                    }

                    types.Add(name, typeof(ListControl));
                    events.Add(name, "ControlSelectionChanged");
                }
                else if (name == "CollisionType")
                {
                    if (value == "0")
                    {
                        properties[name] = "Box:Sphere:Hull:Mesh";
                    }
                    else if (value == "1")
                    {
                        properties[name] = "Sphere:Box:Hull:Mesh";
                    }
                    else if (value == "2")
                    {
                        properties[name] = "Hull:Box:Sphere:Mesh";
                    }
                    else if (value == "3")
                    {
                        properties[name] = "Mesh:Box:Sphere:Hull";
                    }

                    types.Add(name, typeof(ListControl));
                    events.Add(name, "ControlSelectionChanged");
                }
                else if (value == "nil")
                {
                    types.Add(name, typeof(TextControl));
                    properties[name] = "";
                }
                else if (name == "Shader")
                {
                    return;
                }
                else
                {
                    types.Add(name, typeof(TextControl));
                }

                handlers.Add(name, name + "_callback");

                UpdateProperties(selectinon_id, properties, types, events, handlers);
            }
            catch (Exception e)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(e.Message + e.StackTrace));
            }
        }

        internal void LoadBrushInfo(int brush)
        {
            try
            {
                Dictionary<string, object> properties = new Dictionary<string, object>();
                Dictionary<string, Type> types = new Dictionary<string, Type>();
                Dictionary<string, string> events = new Dictionary<string, string>();
                Dictionary<string, string> handlers = new Dictionary<string, string>();

                switch (brush)
                {
                    case 1:
                        properties.Add("Mode", "Paint:Erase");
                        types.Add("Mode", typeof(ListControl));
                        events.Add("Mode", "ControlSelectionChanged");
                        handlers.Add("Mode", "BrushMode_callback");

                        properties.Add("Channels", "Red:Green:Blue:RGB");
                        types.Add("Channels", typeof(ListControl));
                        events.Add("Channels", "ControlSelectionChanged");
                        handlers.Add("Channels", "BrushChannels_callback");
                        break;
                    case 2:
                        properties.Add("Mode", "Add:Subtract:Flattern:Smooth");
                        types.Add("Mode", typeof(ListControl));
                        events.Add("Mode", "ControlSelectionChanged");
                        handlers.Add("Mode", "SculptMode_callback");
                        break;
                }

                properties.Add("Opacity", "1");
                types.Add("Opacity", typeof(LabelControl));
                events.Add("Opacity", "TextBoxTextChanged");
                handlers.Add("Opacity", "Opacity_callback");

                properties.Add("Falloff", "1");
                types.Add("Falloff", typeof(LabelControl));
                events.Add("Falloff", "TextBoxTextChanged");
                handlers.Add("Falloff", "Falloff_callback");

                properties.Add("Size", "1");
                types.Add("Size", typeof(LabelControl));
                events.Add("Size", "TextBoxTextChanged");
                handlers.Add("Size", "Size_callback");

                UpdateProperties(selectinon_id, properties, types, events, handlers);
            }
            catch (Exception e)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(e.Message + e.StackTrace));
            }
        }

        private void EntitiesList_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            try
            {
                if (e.AddedItems.Count > 0)
                {
                    ent_props.Clear();
                    selectinon_id = (e.AddedItems[0] as EntityItem).ID;
                    UIBridge.GetEntityInfo((IntPtr)selectinon_id);
                }
            }
            catch (Exception ee)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(ee.Message));
            }
        }

        internal void SelectEntity(int ID)
        {
            if (ID == 0)
            {
                EntitiesList.SelectedItem = null;
                selectinon_id = 0;
                ent_props.Clear();
            }
            
            for (int i = 0; i < EntitiesList.Items.Count; i++)
            {
                if ((EntitiesList.Items.GetItemAt(i) as EntityItem).ID == ID)
                {
                    selectinon_id = ID;
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
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(ee.Message));
            }
        }

        private void SkyboxSettings_Click(object sender, RoutedEventArgs e)
        {
            SkyboxSettings settings = new SkyboxSettings();
            settings.Show();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            UIBridge.CloseContext();
        }

        private void PhysicsButton_Click(object sender, RoutedEventArgs e)
        {
            UIBridge.TogglePhysics();
        }

        private void ExtraProps_PreviewMouseUp(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            ExtraProps.IsSubmenuOpen = !ExtraProps.IsSubmenuOpen;
        }

        private void MenuItem_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                UIBridge.AddNewEntityProperty((EntitiesList.SelectedItem as EntityItem).ID, Marshal.StringToHGlobalAnsi((sender as System.Windows.Controls.Control).Name));
                ent_props.Clear();
                ExtraProps.IsSubmenuOpen = false;
            }
            catch (Exception ee)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi(ee.Message));
            }
        }

        private void ContextItem_Click(object sender, RoutedEventArgs e)
        {
            if ((sender as System.Windows.Controls.MenuItem).Header.ToString() == "Copy")
            {
                UIBridge.CopyEntity();
            }
            else if ((sender as System.Windows.Controls.MenuItem).Header.ToString() == "Paste")
            {
                UIBridge.PasteEntity();
            }
            else if ((sender as System.Windows.Controls.MenuItem).Header.ToString() == "Delete")
            {
                UIBridge.DeleteEntity();
            }
            else if ((sender as System.Windows.Controls.MenuItem).Header.ToString() == "Snap to terrain")
            {
                UIBridge.SnapEntity();
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
                UIBridge.SaveScene(Marshal.StringToHGlobalAnsi(dlg.FileName));
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
                UIBridge.LoadScene(Marshal.StringToHGlobalAnsi(dlg.FileName));
                ent_props.Clear();
                entities.Clear();
                UIBridge.GetEntitiesList();
            }

            GC.Collect();
        }

        private void TerrainSettings_Click(object sender, RoutedEventArgs e)
        {
            if ((sender as System.Windows.Controls.MenuItem).Header.ToString() == "Generate")
            {
                TerrainSettings settings = new TerrainSettings(0);
                settings.ShowDialog();
            }
            else
            {
                TerrainSettings settings = new TerrainSettings(1);
                settings.ShowDialog();
            }
        }

        private void BrushButton_Click(object sender, RoutedEventArgs e)
        {
            ent_props.Clear();
            UIBridge.ToggleBrush(1);
            LoadBrushInfo(1);
        }

        private void SCulptButton_Click(object sender, RoutedEventArgs e)
        {
            ent_props.Clear();
            UIBridge.ToggleBrush(2);
            LoadBrushInfo(2);
        }

        private void Window_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key == System.Windows.Input.Key.LeftCtrl)
            {
                UIBridge.ControlKey(true);
            }
            else if (e.Key == System.Windows.Input.Key.Escape)
            {
                UIBridge.EscKey(true);
            }
            else if (e.Key == System.Windows.Input.Key.Tab)
            {
                UIBridge.TabKey(true);
            }
            else if (e.Key == System.Windows.Input.Key.S)
            {
                //UIBridge.SKey(true);
            }
        }

        private void Window_KeyUp(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key == System.Windows.Input.Key.LeftCtrl)
            {
                UIBridge.ControlKey(false);
            }
            else if (e.Key == System.Windows.Input.Key.Escape)
            {
                UIBridge.EscKey(false);
            }
            else if (e.Key == System.Windows.Input.Key.Tab)
            {
                UIBridge.TabKey(false);
            }
            else if (e.Key == System.Windows.Input.Key.S)
            {
                //UIBridge.SKey(false);
            }
        }

        private void LightingButton_Click(object sender, RoutedEventArgs e)
        {
            UIBridge.ToggleLighting();
        }

        private void SunMenu_Click(object sender, RoutedEventArgs e)
        {
            DirectionalLightSettings settings = new DirectionalLightSettings();
            settings.Show();
        }

        private void Window_SizeChanged(object sender, SizeChangedEventArgs e)
        {

        }

        private void MenuItem_Click_1(object sender, RoutedEventArgs e)
        {
            System.Windows.MessageBox.Show("Esc - Free camera mode \nTab - Character mode\nMouse scroll - Control brush size\nCtrl+Mouse scroll - Control brush opacity\nCtrl+S - Save", "Help", MessageBoxButton.OK, MessageBoxImage.Information);
        }

        private void SelectTool_Click(object sender, RoutedEventArgs e)
        {
            UIBridge.ResetTools();
        }

        private void SearchBar_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            if (SearchBar.IsKeyboardFocused)
            {
                entities_filter.Clear();
                string filter = SearchBar.Text.Replace("Search...", "");

                for (int i = 0; i < entities.Count; i++)
                {
                    if ((entities[i] as EntityItem).EntityName.ToLower().Contains(filter) || filter == "")
                    {
                        entities_filter.Add((entities[i] as EntityItem));
                    }
                }
            }
        }

        private void SearchBar_GotFocus(object sender, RoutedEventArgs e)
        {
            if (SearchBar.Text == "Search...")
            {
                SearchBar.Text = "";
            }
        }

        private void SearchBar_LostFocus(object sender, RoutedEventArgs e)
        {
            if (SearchBar.Text == "")
            {
                SearchBar.Text = "Search...";
            }
        }

        private void CharacterButton_Click(object sender, RoutedEventArgs e)
        {
            UIBridge.TabKey(true);
        }

        private void NewLevelBtn_Click(object sender, RoutedEventArgs e)
        {
            UIBridge.ClearScene();
        }

        private void GridSplitter_DragDelta(object sender, System.Windows.Controls.Primitives.DragDeltaEventArgs e)
        {
            OutputConsole.Height -= e.VerticalChange;
        }
    };
}