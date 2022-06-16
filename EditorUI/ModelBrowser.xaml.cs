using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows;
using System.Diagnostics;

enum SortType
{
    ID_ASC = 0,
    ID_DESC = 1,
    NAME_ASC = 2,
    NAME_DESC = 3
}

namespace EditorUI
{
    public partial class ModelBrowser : Window, EditorWindow
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
        IntPtr child_hwnd;
        System.Windows.Forms.Panel panel = new System.Windows.Forms.Panel();
        string loaded_mesh = "";
        Dictionary<string, System.Windows.Controls.Control> LoadedAssets = new Dictionary<string, System.Windows.Controls.Control>();
        SortedDictionary<int, string> Materials = new SortedDictionary<int, string>();
        SortType sorting = SortType.ID_ASC;
        bool b_fullpath = false;
        string missing_texture = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + @"\Content\Editor\MissingTexture.png";
        bool is_default_name = true;
        string default_name = "";
        string content_folder = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + @"\Content\";

        public ModelBrowser(IntPtr p)
        {
            pOwner = p;
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

            LoadData();
        }

        public void ParentRender(IntPtr child)
        {
            child_hwnd = child;
            SetParent(child, panel.Handle);
            UpdateChildPosition();
        }

        private void LoadData()
        {
            try
            {
                string[] models_files = System.IO.Directory.GetFiles(content_folder, "*.gmf", System.IO.SearchOption.AllDirectories);

                ClearBrowser();
                foreach (string model_file in models_files)
                {
                    if (model_file.ToLower().Contains(SearchBar.Text.ToLower()) || SearchBar.Text == "" || SearchBar.Text == "Search...")
                    {
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

                        if (!LoadedAssets.ContainsKey(model_file))
                        {
                            LoadedAssets.Add(model_file, item);
                            Browser.Items.Add(item);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                SendMessage(pOwner, 0x1204, IntPtr.Zero, Marshal.StringToHGlobalAnsi("Exception occured in " + new StackFrame(1, true).GetMethod().Name + ": " + e.Message));
                SendMessage(pOwner, 0x0010, IntPtr.Zero, IntPtr.Zero);
            }
        }

        internal void LoadDataAsync()
        {
            LoadData();
        }

        private void ClearBrowser()
        {
            if (Browser.Items.Count > 0)
            {
                Browser.SelectedIndex = -1;
                Browser.Items.Clear();
                LoadedAssets.Clear();
                return;
            }
        }

        public void UpdateChildPosition()
        {
            if (child_hwnd != null)
            {
                SetWindowPos(child_hwnd, IntPtr.Zero, 0, 0, panel.Width, panel.Height, 0);
            }
        }

        private void MdlBrowser_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
        }

        private void BtnLoadMdl_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "obj files (*.obj)|*.obj";
            openFileDialog.FilterIndex = 1;
            openFileDialog.InitialDirectory = content_folder;
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                Materials.Clear();
                loaded_mesh = openFileDialog.FileName;
                MeshPath.Text = loaded_mesh;
                MeshPath.ToolTip = loaded_mesh;
                default_name = openFileDialog.SafeFileName.Split('.')[0];
                is_default_name = true;
                IdBox.Text = default_name;

                SendMessage(pOwner, 0x1200, IntPtr.Zero, Marshal.StringToHGlobalAnsi(openFileDialog.FileName));
            }

            GC.Collect();
        }

        private void MdlBrowser_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateChildPosition();
        }

        internal void AddMaterialToTheTable(string string_names)
        {
            MaterialsPanel.Children.Clear();

            int mat_index = 0;
            string[] material_names = string_names.Split('\\');

            bool isNumeric = int.TryParse(IdBox.Text, out int id);
            if (!isNumeric) id = -1;

            foreach (string material_name in material_names)
            {
                if (material_name != String.Empty)
                {
                    if (!Materials.ContainsKey(mat_index))
                    {
                        Materials.Add(mat_index, missing_texture);
                        SendMessage(pOwner, 0x1202, (IntPtr)mat_index, Marshal.StringToHGlobalAnsi(Materials[mat_index]));
                    }

                    MaterialInput material_panel = new MaterialInput(pOwner);
                    material_panel.NameTextBlock.Text = material_name;
                    material_panel.MaterialPath.Text = Materials[mat_index];
                    material_panel.MaterialPath.ToolTip = Materials[mat_index];
                    material_panel.event_load_material += LoadMaterial;
                    material_panel.material_index = mat_index++;
                    MaterialsPanel.Children.Add(material_panel);
                    System.Windows.Controls.DockPanel.SetDock(material_panel, System.Windows.Controls.Dock.Top);
                }
            }
        }

        private void BtnCreate_Click(object sender, RoutedEventArgs e)
        {
            string path = loaded_mesh;

            for (int i = loaded_mesh.Length - 1; i > 0; i--)
            {
                if (loaded_mesh[i] != '\\')
                {
                    continue;
                }
                else
                {
                    path = loaded_mesh.Substring(0, i) + '\\';
                    break;
                }
            }

            /*GrayEngine model file*/
            string currentfile = "<mesh>" + loaded_mesh + "<|mesh>";
            System.IO.FileStream model_file = new System.IO.FileStream(path + IdBox.Text + ".gmf", System.IO.FileMode.Create);
            model_file.Write(System.Text.Encoding.ASCII.GetBytes(currentfile), 0, System.Text.Encoding.ASCII.GetBytes(currentfile).Length);

            for (int itt = 0; itt < Materials.Count; itt++)
            {
                currentfile = "<texture>" + Materials[itt] + "<|texture>";
                model_file.Write(System.Text.Encoding.ASCII.GetBytes(currentfile), 0, System.Text.Encoding.ASCII.GetBytes(currentfile).Length);
            }
            model_file.Close();

            LoadData();
            if (LoadedAssets.ContainsKey(loaded_mesh))
                Browser.SelectedIndex = LoadedAssets.Keys.ToList().IndexOf(loaded_mesh);
        }

        private void LoadMaterial(object sender, EventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "png files (*.png)|*.png";
            openFileDialog.FilterIndex = 1;
            openFileDialog.InitialDirectory = content_folder;
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                SendMessage(pOwner, 0x1202, (IntPtr)((MaterialInput)sender).material_index, Marshal.StringToHGlobalAnsi(openFileDialog.FileName));
                Materials[((MaterialInput)sender).material_index] = openFileDialog.FileName;
                ((MaterialInput)sender).MaterialPath.Text = openFileDialog.FileName;
                ((MaterialInput)sender).MaterialPath.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void Browser_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            try
            {
                if (Browser.SelectedIndex < 0) return;

                System.IO.FileStream io_file = new System.IO.FileStream(((BrowserItem)Browser.SelectedItem).ToolTip.ToString(), System.IO.FileMode.Open, System.IO.FileAccess.Read);
                System.Threading.CancellationTokenSource source = new System.Threading.CancellationTokenSource();

                byte[] buffer = new byte[io_file.Length];
                io_file.Read(buffer, 0, (int)io_file.Length);
                string file_path = String.Empty;
                string file_type = String.Empty;
                bool comma = false;
                string cur_mesh = String.Empty;
                string cur_texs = String.Empty;
                string[] name_cut = io_file.Name.Split('\\');
                Materials.Clear();

                foreach (char chr in buffer)
                {
                    if (chr == '<')
                    {
                        if (file_path != String.Empty)
                        {
                            if (file_type == "<mesh>")
                            {
                                cur_mesh = file_path;
                            }
                            else if (file_type == "<texture>")
                            {
                                cur_texs += file_path + '|';
                                Materials.Add(Materials.Count, file_path);
                            }
                        }

                        file_type = String.Empty;
                        file_path = string.Empty;
                        comma = true;
                        file_type += chr;
                    }
                    else if (chr == '>')
                    {
                        file_type += chr;
                        comma = false;
                    }
                    else if (comma)
                    {
                        file_type += chr;
                    }
                    else
                    {
                        file_path += chr;
                    }
                }
                io_file.Close();

                SendMessage(pOwner, 0x1203, Marshal.StringToHGlobalAnsi(cur_mesh), Marshal.StringToHGlobalAnsi(cur_texs));
                IdBox.Text = name_cut[name_cut.Length - 1].Split('.')[0];
                loaded_mesh = cur_mesh;
                MeshPath.Text = loaded_mesh;
                MeshPath.ToolTip = loaded_mesh;
            }
            catch (Exception exc)
            {
                SendMessage(pOwner, 0x1204, IntPtr.Zero, Marshal.StringToHGlobalAnsi("Exception occured in " + new StackFrame(1, true).GetMethod().Name + ": " + exc.Message));
                SendMessage(pOwner, 0x0010, IntPtr.Zero, IntPtr.Zero);
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

        string GetSortString()
        {
            switch (sorting)
            {
                case SortType.ID_ASC:
                    return "Id ASC";
                case SortType.ID_DESC:
                    return "Id DESC";
                case SortType.NAME_ASC:
                    return "Mesh ASC";
                case SortType.NAME_DESC:
                    return "Mesh DESC";
                default:
                    return "Id ASC";
            }
        }

        private void IdBox_TextInput(object sender, System.Windows.Input.TextCompositionEventArgs e)
        {
            string restriction = @"#%&{}\\<>*?/ $!':@+`|=" + '"';
            ((System.Windows.Controls.TextBox)sender).Undo();

            
            if (!restriction.Contains(e.Text))
            {
                ((System.Windows.Controls.TextBox)sender).Text += e.Text;
            }

            is_default_name = false;
        }

        private void TabCtr_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            ((System.Windows.Controls.Control)sender).Focus();
        }

        private void IdBox_GotFocus(object sender, RoutedEventArgs e)
        {
            if (is_default_name)
            {
                ((System.Windows.Controls.TextBox)sender).Text = String.Empty;
            }
        }

        private void IdBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (is_default_name)
            {
                ((System.Windows.Controls.TextBox)sender).Text = default_name;
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            LoadData();
        }
    }
}
