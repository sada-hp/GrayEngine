using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows;
using System.Diagnostics;

enum SortType
{
    NAME_ASC = 0,
    NAME_DESC = 1
}

namespace EditorUI
{
    public partial class ModelBrowser : Window, EditorWindow
    {
        IntPtr child_hwnd;
        bool b_fullpath = false;
        string loaded_mesh = "";
        string loaded_collision = "";
        string default_name = "";
        SortType sorting = SortType.NAME_ASC;
        System.Windows.Forms.Panel panel = new System.Windows.Forms.Panel();
        SortedDictionary<int, string> Materials = new SortedDictionary<int, string>();
        SortedDictionary<int, string> Normals = new SortedDictionary<int, string>();
        SortedDictionary<string, System.Windows.Controls.Control> LoadedAssets = new SortedDictionary<string, System.Windows.Controls.Control>();
        public static string distr_location = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
        public static string content_folder = distr_location + @"\Content\";
        public static string missing_texture = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + @"\Content\Editor\MissingTexture.png";

        public ModelBrowser()
        {
            InitializeComponent();
            panel.CreateControl();
            panel.Dock = System.Windows.Forms.DockStyle.Fill;
            panel.BackColor = System.Drawing.Color.Black;
            panel.BorderStyle = System.Windows.Forms.BorderStyle.None;
            panel.Margin = new System.Windows.Forms.Padding(0);
            panel.BorderStyle = System.Windows.Forms.BorderStyle.None;
            FormHost.Child = panel;
            IdBox.AddHandler(System.Windows.Controls.TextBox.TextInputEvent, new System.Windows.Input.TextCompositionEventHandler(IdBox_TextInput), true);
            RefreshButton.Content = "\u21BA";
            MeshNameBar.Content = "Name \u2B9F";
            LoadData();
        }

        public void ParentRender(IntPtr child)
        {
            child_hwnd = child;
            UIBridge.SetParent(child, panel.Handle);
            UpdateChildPosition();
        }

        private void LoadData()
        {
            try
            {
                string[] models_files = System.IO.Directory.GetFiles(content_folder, "*.gmf", System.IO.SearchOption.AllDirectories);
                List<string> list_models = models_files.ToList();
                list_models.Sort();
                if (sorting == SortType.NAME_ASC)
                    list_models.Reverse();

                ClearBrowser();
                foreach (string model_file in list_models)
                {
                    if (model_file.ToLower().Contains(SearchBar.Text.ToLower()) || SearchBar.Text == "" || SearchBar.Text == "Search...")
                    {
                        System.IO.FileStream file = new System.IO.FileStream(model_file, System.IO.FileMode.Open);
                        byte[] byte_array = new byte[file.Length];
                        file.Read(byte_array, 0, byte_array.Length);
                        string oper = "";
                        string mode = "";
                        string mesh_path = "";
                        string collision_path = "";
                        bool writing = false;

                        foreach (char chr in byte_array)
                        {
                            if (chr == ' ' || chr == '\n')
                            {
                                continue;
                            }
                            else if (chr == '{')
                            {
                                writing = true;
                                oper = "";
                            }
                            else if (chr == '}')
                            {
                                if (mode.Contains("mesh"))
                                {
                                    mesh_path = oper.Trim();
                                }
                                else if (mode.Contains("collision"))
                                {
                                    collision_path = oper.Trim();
                                }
                                writing = false;
                                oper = "";
                                mode = "";
                            }
                            else
                            {
                                if (writing)
                                {
                                    oper += chr;
                                }
                                else
                                {
                                    mode += chr;
                                }
                            }
                        }
                        file.Close();
                        file.Dispose();

                        BrowserItem item;
                        string[] name_cut = model_file.Split('\\');

                        if (!b_fullpath)
                        {
                            item = new BrowserItem("...\\" + name_cut[name_cut.Length - 2] + '\\' + name_cut[name_cut.Length - 1]);
                        }
                        else
                        {
                            item = new BrowserItem(model_file);
                        }

                        item.Padding = new System.Windows.Thickness(5);
                        item.ToolTip = model_file;
                        item.mesh_path = mesh_path;
                        item.collision_path = collision_path;

                        if (!LoadedAssets.ContainsKey(model_file))
                        {
                            LoadedAssets.Add(model_file, item);
                            Browser.Items.Add(item);
                            item.MainControl.AddHandler(System.Windows.Controls.Label.MouseDoubleClickEvent, new System.Windows.Input.MouseButtonEventHandler(BrowserItem_DoubleClick));
                        }
                    }
                }
            }
            catch (Exception e)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi("Exception occured in " + new StackFrame(1, true).GetMethod().Name + ": " + e.Message));
                UIBridge.CloseContext();
            }
        }

        internal void ReadModel(string filepath)
        {
            try
            {
                Normals.Clear();
                Materials.Clear();
                System.IO.StreamReader file = new System.IO.StreamReader(filepath);
                int mode = -1;

                while (!file.EndOfStream)
                {
                    string stream = file.ReadLine();
                    if (stream.Contains("mesh"))
                    {
                        mode = 0;
                    }
                    else if (stream.Contains("collision"))
                    {
                        mode = 1;
                    }
                    else if (stream.Contains("textures"))
                    {
                        mode = 2;
                    }
                    else if (stream.Contains("normals"))
                    {
                        mode = 3;
                    }
                    else if (stream != "{" && stream != "}")
                    {
                        switch (mode)
                        {
                            case 0:
                                loaded_mesh = stream.Trim();
                                break;
                            case 1:
                                loaded_collision = stream.Trim();
                                break;
                            case 2:
                                Materials[Materials.Count] = stream.Trim();
                                break;
                            case 3:
                                Normals[Normals.Count] = stream.Trim();
                                break;
                        }
                    }
                }
            }
            catch (Exception e)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi("Exception occured in " + new StackFrame(1, true).GetMethod().Name + ": " + e.Message));
                UIBridge.CloseContext();
            }
        }

        private void ClearBrowser()
        {
            if (Browser.Items.Count > 0)
            {
                Browser.SelectedIndex = -1;
                foreach (BrowserItem item in Browser.Items)
                {
                    item.MouseDoubleClick -= BrowserItem_DoubleClick;
                }
                Browser.Items.Clear();
                LoadedAssets.Clear();
                return;
            }
        }

        public void UpdateChildPosition()
        {
            if (child_hwnd != IntPtr.Zero)
            {
                UIBridge.SetWindowPos(child_hwnd, IntPtr.Zero, 0, 0, panel.Width, panel.Height, 0);
            }
        }

        private void MdlBrowser_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            child_hwnd = IntPtr.Zero;

            ClearBrowser();
            UIBridge.CloseModelBrowser();
            LoadedAssets.Clear();
            Materials.Clear();
            Normals.Clear();
            IdBox.TextInput -= IdBox_TextInput;

            GC.Collect();
        }

        private void BtnLoadMdl_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "mesh files (*.obj; *.fbx)|*.obj;*.fbx";
            openFileDialog.InitialDirectory = content_folder;
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                Materials.Clear();
                Normals.Clear();
                loaded_mesh = openFileDialog.FileName;
                MeshPath.Text = loaded_mesh;
                MeshPath.ToolTip = loaded_mesh;
                default_name = openFileDialog.SafeFileName.Split('.')[0];
                IdBox.Text = default_name;

                if (ColPath.Text == "")
                {
                    loaded_collision = openFileDialog.FileName;
                    ColPath.Text = loaded_collision;
                    ColPath.ToolTip = loaded_collision;
                }
                Materials[0] = missing_texture;

                UIBridge.LoadObject(Marshal.StringToHGlobalAnsi(openFileDialog.FileName), Marshal.StringToHGlobalAnsi(missing_texture + '|'));
            }

            GC.Collect();
        }

        private void MdlBrowser_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateChildPosition();
        }

        private void LoadMaterial(object sender, EventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "png files (*.png)|*.png";
            openFileDialog.InitialDirectory = content_folder;
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                Materials[((MaterialInput)sender).material_index] = openFileDialog.FileName;
                ((MaterialInput)sender).MaterialPath.Text = openFileDialog.FileName;
                ((MaterialInput)sender).MaterialPath.ToolTip = openFileDialog.FileName;

                UIBridge.AssignTextures(Marshal.StringToHGlobalAnsi(GetTexturesString()));
            }

            GC.Collect();
        }

        private void LoadNormals(object sender, EventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "png files (*.png)|*.png";
            openFileDialog.InitialDirectory = content_folder;
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                Normals[((MaterialInput)sender).material_index] = openFileDialog.FileName;
                ((MaterialInput)sender).NormalPath.Text = openFileDialog.FileName;
                ((MaterialInput)sender).NormalPath.ToolTip = openFileDialog.FileName;

                //UIBridge.AssignTextures(Marshal.StringToHGlobalAnsi(GetTexturesString()));
            }

            GC.Collect();
        }

        internal void AddMaterialToTheTable(string material_names, int redraw)
        {
            MaterialsPanel.Children.Clear();

            int mat_index = 0;
            string[] materials = material_names.Split('|');

            foreach (string material_name in materials)
            {
                if (material_name != String.Empty)
                {
                    MaterialInput material_panel = new MaterialInput();
                    material_panel.NameTextBlock.Text = material_name;
                    if (mat_index < Materials.Count)
                    {
                        material_panel.MaterialPath.Text = Materials[mat_index];
                        material_panel.MaterialPath.ToolTip = Materials[mat_index];
                    }
                    else
                    {
                        Materials[mat_index] = missing_texture;
                        material_panel.MaterialPath.Text = Materials[mat_index];
                        material_panel.MaterialPath.ToolTip = Materials[mat_index];
                    }
                    if (mat_index < Normals.Count)
                    {
                        Normals[mat_index] = Normals[mat_index];
                        material_panel.NormalPath.Text = Normals[mat_index];
                        material_panel.NormalPath.ToolTip = Normals[mat_index];
                    }
                    else
                    {
                        Normals[mat_index] = "";
                        material_panel.NormalPath.Text = Normals[mat_index];
                        material_panel.NormalPath.ToolTip = Normals[mat_index];
                    }
                    material_panel.event_load_material += LoadMaterial;
                    material_panel.event_load_normal += LoadNormals;
                    material_panel.material_index = mat_index++;
                    MaterialsPanel.Children.Add(material_panel);
                    System.Windows.Controls.DockPanel.SetDock(material_panel, System.Windows.Controls.Dock.Top);
                }
            }
            if (redraw == 1)
                UIBridge.AssignTextures(Marshal.StringToHGlobalAnsi(GetTexturesString()));

            GC.Collect();
        }

        private string GetTexturesString()
        {
            string res = "";
            foreach (var tex in Materials)
            {
                string itt = tex.Value;
                if (itt == "")
                {
                    itt = missing_texture;
                }
                else if (itt.Length < distr_location.Length)
                {
                    itt = distr_location + '\\' + itt;
                }

                res += itt + '|';
            }

            return res;
        }

        private void BtnCreate_Click(object sender, RoutedEventArgs e)
        {
            string material_string = "";
            foreach (var texture in Materials.Values)
            {
                string texture_val = texture.StartsWith(distr_location) ? texture.Remove(0, distr_location.Length + 1) : texture;
                material_string += ":Color:" + texture_val + "|";
            }
            foreach (var normal in Normals.Values)
            {
                string normal_val = normal.StartsWith(distr_location) ? normal.Remove(0, distr_location.Length + 1) : normal;
                material_string += ":Normal:" + normal_val + "|";
            }

            string file = loaded_mesh.Substring(0, loaded_mesh.LastIndexOf('\\')) + '\\' + IdBox.Text + ".gmf";
            string mesh = loaded_mesh.StartsWith(distr_location) ? loaded_mesh.Remove(0, distr_location.Length + 1) : loaded_mesh;
            string collision = loaded_collision.StartsWith(distr_location) ? loaded_collision.Remove(0, distr_location.Length + 1) : loaded_collision;
            UIBridge.CreateModelFile(Marshal.StringToHGlobalAnsi(file), Marshal.StringToHGlobalAnsi(mesh), Marshal.StringToHGlobalAnsi(collision), Marshal.StringToHGlobalAnsi(material_string));
            LoadData();
        }

        private void Browser_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            try
            {
                if (Browser.SelectedIndex < 0) return;

                ReadModel(((BrowserItem)Browser.SelectedItem).ToolTip.ToString());
                IdBox.Text = ((BrowserItem)Browser.SelectedItem).ToolTip.ToString().Split('\\').Last().Split('.')[0];
                MeshPath.Text = loaded_mesh;
                MeshPath.ToolTip = loaded_mesh;
                ColPath.Text = loaded_collision;
                ColPath.ToolTip = loaded_collision;
                UIBridge.LoadModelFile(Marshal.StringToHGlobalAnsi(((BrowserItem)Browser.SelectedItem).ToolTip.ToString()));
                GC.Collect();
            }
            catch (Exception exc)
            {
                UIBridge.LogMessage(Marshal.StringToHGlobalAnsi("Exception occured in " + new StackFrame(1, true).GetMethod().Name + ": " + exc.Message));
                UIBridge.CloseContext();
            }
        }

        private void SearchBar_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            if (SearchBar.IsFocused)
            {
                LoadData();
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

        private void FormHost_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateChildPosition();
        }

        private void GridSplitter_DragDelta(object sender, System.Windows.Controls.Primitives.DragDeltaEventArgs e)
        {
            TabCtr.Width += e.HorizontalChange;
        }

        private void MeshNameBar_Click(object sender, RoutedEventArgs e)
        {
            switch (sorting)
            {
                case SortType.NAME_ASC:
                    sorting = SortType.NAME_DESC;
                    MeshNameBar.Content = "Name \u2B9D";
                    break;
                case SortType.NAME_DESC:
                    sorting = SortType.NAME_ASC;
                    MeshNameBar.Content = "Name \u2B9F";
                    break;
                default:
                    sorting = SortType.NAME_ASC;
                    MeshNameBar.Content = "Name \u2B9F";
                    break;
            }

            LoadData();
        }

        private void ExpandBtn_Click(object sender, RoutedEventArgs e)
        {
            b_fullpath = !b_fullpath;

            ExpandBtn.Content = b_fullpath ? "-" : "+";
            ExpandBtn.ToolTip = b_fullpath ? "Condense path" : "Expand path";

            LoadData();
        }

        private void IdBox_TextInput(object sender, System.Windows.Input.TextCompositionEventArgs e)
        {
            string restriction = @"#%&{}\\<>*?/ $!':@+`|=." + '"';
            ((System.Windows.Controls.TextBox)sender).Undo();

            if (!restriction.Contains(e.Text))
            {
                ((System.Windows.Controls.TextBox)sender).Text += e.Text;
                ((System.Windows.Controls.TextBox)sender).ScrollToEnd();
                ((System.Windows.Controls.TextBox)sender).CaretIndex = ((System.Windows.Controls.TextBox)sender).Text.Length;
            }
        }

        private void TabCtr_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            ((System.Windows.Controls.Control)sender).Focus();
        }

        private void IdBox_GotFocus(object sender, RoutedEventArgs e)
        {
            if (IdBox.Text == loaded_mesh.Split('\\').Last().Split('.')[0])
            {
                ((System.Windows.Controls.TextBox)sender).Text = String.Empty;
            }
        }

        private void IdBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (IdBox.Text == loaded_mesh.Split('\\').Last().Split('.')[0])
            {
                ((System.Windows.Controls.TextBox)sender).Text = default_name;
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            LoadData();
        }

        private void BrowserItem_DoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            UIBridge.AddToTheScene(Marshal.StringToHGlobalAnsi(((BrowserItem)Browser.SelectedItem).ToolTip.ToString()));
            Close();
        }

        private void MdlBrowser_Closed(object sender, EventArgs e)
        {
            panel.Dispose();
            FormHost.Dispose();
            UIBridge.DestroyUserInterface(1);
        }

        private void BtnLoadCol_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "mesh files (*.obj; *.fbx)|*.obj;*.fbx";
            openFileDialog.InitialDirectory = content_folder;
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                loaded_collision = openFileDialog.FileName;
                ColPath.Text = loaded_collision;
                ColPath.ToolTip = loaded_collision;
            }

            GC.Collect();
        }
    }
}