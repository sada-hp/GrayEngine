using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows;

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
        string connection = String.Empty;
        string loaded_mesh = "";
        Dictionary<string, System.Windows.Controls.Control> LoadedAssets = new Dictionary<string, System.Windows.Controls.Control>();
        SortedDictionary<int, string> Materials = new SortedDictionary<int, string>();
        SortType sorting = SortType.ID_ASC;
        bool b_fullpath = false;
        string missing_texture = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + @"\Content\Editor\MissingTexture.png";

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
            IdBox.AddHandler(System.Windows.Controls.TextBox.TextInputEvent,  new System.Windows.Input.TextCompositionEventHandler(IdBox_TextInput), true);

            connection = @"Data Source=(LocalDB)\MSSQLLocalDB;AttachDbFilename=" + System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + @"\Database\Assets_models.mdf;Integrated Security=True";
            LoadData();

            Id.Content = "Id \u2B9F";
        }

        public void ParentRender(IntPtr child)
        {
            child_hwnd = child;
            SetParent(child, panel.Handle);
            UpdateChildPosition();
        }

        private void ConnectDatabase(string connection_string, string command_string, out System.Data.SqlClient.SqlConnection sql_coonection, out System.Data.SqlClient.SqlCommand sql_command)
        {
            sql_coonection = new System.Data.SqlClient.SqlConnection(connection_string);
            sql_command = new System.Data.SqlClient.SqlCommand();
            sql_command.CommandType = System.Data.CommandType.Text;
            sql_command.CommandText = command_string;
            sql_command.Connection = sql_coonection;

            sql_coonection.Open();
        }

        private void NewDataBaseEntry(string name)
        {
            try
            {
                if (name != "") //TBD: existence check
                {
                    bool isNumeric = int.TryParse(IdBox.Text, out int id);

                    if (!isNumeric) id = -1;

                    ConnectDatabase(connection, "SELECT COUNT(*) FROM Models WHERE Id = " + id + "", out System.Data.SqlClient.SqlConnection sqlConnection, out System.Data.SqlClient.SqlCommand cmd);
                  
                    int count = (int)cmd.ExecuteScalar();

                    if (count == 0)
                    {
                        cmd.Prepare();
                        cmd.CommandText = @"SELECT COUNT(*) FROM Models";
                        id = (int)cmd.ExecuteScalar();
                        cmd.CommandText = @"INSERT INTO Models (id, Mesh) VALUES (@p1, @p2)";
                        cmd.Parameters.Add("@p1", System.Data.SqlDbType.Int).Value = id;
                        cmd.Parameters.Add("@p2", System.Data.SqlDbType.NVarChar, name.Trim().Length).Value = name;
                        cmd.ExecuteNonQuery();

                        for (Int16 itt = 0; itt < Materials.Count; itt++)
                        {
                            cmd.Parameters.Clear();
                            cmd.CommandText = @"INSERT INTO Textures (id, MatInd, TexPath) VALUES (@p1, @p2, @p3)";
                            cmd.Parameters.Add("@p1", System.Data.SqlDbType.Int).Value = id;
                            cmd.Parameters.Add("@p2", System.Data.SqlDbType.TinyInt).Value = itt;
                            cmd.Parameters.Add("@p3", System.Data.SqlDbType.NVarChar, Materials[itt].Length).Value = Materials[itt];
                            cmd.ExecuteNonQuery();
                        }
                    }
                    else
                    {
                        System.Windows.MessageBoxButton buttons = System.Windows.MessageBoxButton.YesNo;
                        var res = System.Windows.MessageBox.Show("Entry already exists. Override?", "Warning", buttons, System.Windows.MessageBoxImage.Question);
                        if (res == MessageBoxResult.Yes)
                        {
                            cmd.Prepare();
                            cmd.CommandText = @"UPDATE Models SET MESH = '" + name + "' WHERE id = '" + id + "'";
                            cmd.ExecuteNonQuery();

                            for (Int16 itt = 0; itt < Materials.Count; itt++)
                            {
                                cmd.Parameters.Clear();
                                cmd.CommandText = @"UPDATE Textures SET TexPath = '" + Materials[itt] + "' WHERE id = " + id + " AND MatInd = " + itt;
                                cmd.ExecuteNonQuery();
                            }
                        }
                    }

                    sqlConnection.Close();
                    SendMessage(pOwner, 0x1204, IntPtr.Zero, Marshal.StringToHGlobalAnsi("New database entry"));
                }
                else
                {
                    SendMessage(pOwner, 0x1204, IntPtr.Zero, Marshal.StringToHGlobalAnsi("Emty mesh file was provided to database!"));
                }

                LoadData();
            }
            catch (Exception e)
            {
                SendMessage(pOwner, 0x1204, IntPtr.Zero, Marshal.StringToHGlobalAnsi("Exception occured : " + e.Message));
            }
        }

        private void LoadData()
        {
            try
            {
                ClearBrowser();

                string filter = SearchBar.Text.Replace("Search...", String.Empty);

                ConnectDatabase(connection, "SELECT * FROM Models WHERE Mesh LIKE '%" + filter + "%' ORDER BY " + GetSortString(), out System.Data.SqlClient.SqlConnection sqlConnection, out System.Data.SqlClient.SqlCommand cmd);

                var reader = cmd.ExecuteReader();

                while (reader.Read())
                {
                    BrowserItem item;
                    string name = reader.GetString(1);
                    string[] name_cut = name.Split('\\');

                    if (!b_fullpath)
                    {

                        item = new BrowserItem(Convert.ToString(reader.GetInt32(0)), "...\\" + name_cut[name_cut.Length - 2] + '\\' + name_cut[name_cut.Length - 1]);
                    }
                    else
                    {
                        item = new BrowserItem(Convert.ToString(reader.GetInt32(0)), name);
                    }

                    item.Padding = new System.Windows.Thickness(5);
                    item.ToolTip = name;

                    if (!LoadedAssets.ContainsKey(name))
                    {
                        LoadedAssets.Add(name, item);
                        Browser.Items.Add(item);
                    }
                }

                sqlConnection.Close();
            }
            catch (Exception e)
            {
                SendMessage(pOwner, 0x1204, IntPtr.Zero, Marshal.StringToHGlobalAnsi("Exception occured : " + e.Message));
            }
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

        private void ClearDataBase()
        {
            ConnectDatabase(connection, @"TRUNCATE TABLE Models", out System.Data.SqlClient.SqlConnection sqlConnection, out System.Data.SqlClient.SqlCommand cmd);

            cmd.ExecuteNonQuery();
            sqlConnection.Close();
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
            //ClearDataBase(); //To Be Removed
        }

        private void BtnLoadMdl_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "obj files (*.obj)|*.obj";
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                loaded_mesh = openFileDialog.FileName;
                MeshPath.Text = loaded_mesh;
                MeshPath.ToolTip = loaded_mesh;
                IdBox.Text = "Auto";

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
            Materials.Clear();

            int mat_index = 0;
            string[] material_names = string_names.Split('\\');

            bool isNumeric = int.TryParse(IdBox.Text, out int id);
            if (!isNumeric) id = -1;

            ConnectDatabase(connection, @"SELECT TexPath FROM Textures WHERE Id = " + id + "", out System.Data.SqlClient.SqlConnection sqlConnection, out System.Data.SqlClient.SqlCommand cmd);
            var reader = cmd.ExecuteReader();

            foreach (string material_name in material_names)
            {
                if (material_name != String.Empty)
                {
                    MaterialInput material_panel = new MaterialInput(pOwner);

                    material_panel.NameTextBlock.Text = material_name;
                    System.Windows.Controls.DockPanel.SetDock(material_panel, System.Windows.Controls.Dock.Top);

                    if (reader.Read())
                    {
                        Materials.Add(mat_index, reader.GetString(0));
                        material_panel.MaterialPath.Text = reader.GetString(0);
                        material_panel.MaterialPath.ToolTip = reader.GetString(0);
                    }
                    else
                        Materials.Add(mat_index, missing_texture);

                    SendMessage(pOwner, 0x1202, (IntPtr)mat_index, Marshal.StringToHGlobalAnsi(Materials[mat_index]));
                    material_panel.event_load_material += LoadMaterial;
                    material_panel.material_index = mat_index++;

                    MaterialsPanel.Children.Add(material_panel);
                }
            }
        }

        private void BtnCreate_Click(object sender, RoutedEventArgs e)
        {
            for (int itt = 0; itt < Materials.Count; itt++)
            {
                if (Materials[itt] == String.Empty)
                {
                    Materials[itt] = missing_texture;
                }
            }

            NewDataBaseEntry(loaded_mesh);

            Browser.SelectedIndex = LoadedAssets.Keys.ToList().IndexOf(loaded_mesh);
        }

        private void LoadMaterial(object sender, EventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "png files (*.png)|*.png";
            openFileDialog.FilterIndex = 1;
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
            if (Browser.SelectedItem != null)
            {
                string materials = String.Empty;
                string model = (Browser.SelectedItem as BrowserItem).ToolTip.ToString().Trim();
                string id = (Browser.SelectedItem as BrowserItem).Id_Label.Content.ToString().Trim();

                ConnectDatabase(connection, @"SELECT TexPath FROM Textures WHERE Id = " + id + "", out System.Data.SqlClient.SqlConnection sqlConnection, out System.Data.SqlClient.SqlCommand cmd);

                var reader = cmd.ExecuteReader();

                while (reader.Read())
                {
                    materials += reader.GetString(0) + '|';
                }

                SendMessage(pOwner, 0x1203, Marshal.StringToHGlobalAnsi(model), Marshal.StringToHGlobalAnsi(materials));

                IdBox.Text = id;
                loaded_mesh = model;
                MeshPath.Text = loaded_mesh;
                MeshPath.ToolTip = loaded_mesh;

                sqlConnection.Close();
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
                    Id.Content = "Id";
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

        private void Id_Click(object sender, RoutedEventArgs e)
        {
            switch (sorting)
            {
                case SortType.ID_ASC:
                    sorting = SortType.ID_DESC;
                    Id.Content = "Id \u2B9D";
                    break;
                case SortType.ID_DESC:
                    sorting = SortType.ID_ASC;
                    Id.Content = "Id \u2B9F";
                    break;
                default:
                    sorting = SortType.ID_ASC;
                    Id.Content = "Id \u2B9F";
                    MeshNameBar.Content = "Name";
                    break;
            }

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
            ((System.Windows.Controls.TextBox)sender).Undo();

            bool isNumeric = int.TryParse(e.Text, out int id);
            
            
            if (isNumeric)
            {
                if (((System.Windows.Controls.TextBox)sender).Text == "Auto")
                {
                    ((System.Windows.Controls.TextBox)sender).Text = "";
                }

                ((System.Windows.Controls.TextBox)sender).Text += e.Text;
            }
        }

        private void TabCtr_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            ((System.Windows.Controls.Control)sender).Focus();
        }
    }
}
