using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using System.Windows.Forms;

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
        string loaded_material = "";
        Dictionary<string, System.Windows.Controls.ListViewItem> LoadedAssets = new Dictionary<string, System.Windows.Controls.ListViewItem>();
        SortedDictionary<int, string> Materials = new SortedDictionary<int, string>();

        public ModelBrowser(IntPtr p)
        {
            pOwner = p;
            InitializeComponent();
            panel.CreateControl();
            panel.Dock = DockStyle.Fill;
            panel.BackColor = System.Drawing.Color.Black;
            panel.BorderStyle = BorderStyle.None;
            panel.Margin = new System.Windows.Forms.Padding(0);
            panel.BorderStyle = BorderStyle.None;
            FormHost.Child = panel;

            connection = @"Data Source=(LocalDB)\MSSQLLocalDB;AttachDbFilename="+ System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location)  + @"\Database\Assets_models.mdf;Integrated Security=True";
            LoadData();
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

        private void NewDataBaseEntry(string name, string materials_string)
        {
            if (name != "") //TBD: existence check
            {
                ConnectDatabase(connection, "SELECT COUNT(*) FROM Models WHERE Mesh LIKE '%" + name.Trim() + "%'", out System.Data.SqlClient.SqlConnection sqlConnection, out System.Data.SqlClient.SqlCommand cmd);

                int count = (int)cmd.ExecuteScalar();

                if (count == 0)
                {
                    cmd.Prepare();
                    cmd.CommandText = @"INSERT INTO Models (Mesh, Materials) VALUES ('" + name + "', '" + materials_string + "')";
                    cmd.ExecuteNonQuery();
                }
                else
                {
                    System.Windows.MessageBoxButton buttons = System.Windows.MessageBoxButton.YesNo;
                    var res = System.Windows.MessageBox.Show("Entry already exists. Override?", "Warning", buttons, System.Windows.MessageBoxImage.Question);
                    if (res == MessageBoxResult.Yes)
                    {
                        cmd.Prepare();
                        cmd.CommandText = @"UPDATE Models SET Materials = '" + materials_string + "' WHERE Mesh = '" + name + "'";
                        cmd.ExecuteNonQuery();
                    }
                }

                sqlConnection.Close();
            }

            LoadData();
        }

        private void LoadData()
        {
            ClearBrowser();

            string filter = SearchBar.Text.Replace("Search...", String.Empty);
            ConnectDatabase(connection, "SELECT * FROM Models WHERE Mesh LIKE '%" + filter + "%'", out System.Data.SqlClient.SqlConnection sqlConnection, out System.Data.SqlClient.SqlCommand cmd);

            var reader = cmd.ExecuteReader();

            while (reader.Read())
            {
                System.Windows.Controls.ListViewItem item = new System.Windows.Controls.ListViewItem();
                string name = reader.GetString(0);
                string[] name_cut = name.Split('\\');
                item.Content = name_cut[name_cut.Length - 2] + '\\' + name_cut[name_cut.Length - 1] + '\n';
                item.ToolTip = name;

                if (!LoadedAssets.ContainsKey(name))
                {
                    LoadedAssets.Add(name, item);
                    Browser.Items.Add(item);
                }
            }

            sqlConnection.Close();
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
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "obj files (*.obj)|*.obj";
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                loaded_mesh = openFileDialog.FileName;
                loaded_material = String.Empty;
                MeshPath.Text = loaded_mesh;

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
            string[] texture_names = loaded_material.Split('|');
            
            foreach (string material_name in material_names)
            {
                if (material_name != String.Empty)
                {
                    MaterialInput material_panel = new MaterialInput(pOwner);

                    material_panel.NameTextBlock.Text = material_name;
                    System.Windows.Controls.DockPanel.SetDock(material_panel, System.Windows.Controls.Dock.Top);

                    if (mat_index < texture_names.Length)
                    {
                        Materials.Add(mat_index, texture_names[mat_index]);
                        material_panel.MaterialPath.Text = texture_names[mat_index];
                    }
                    else
                        Materials.Add(mat_index, String.Empty);

                    material_panel.event_load_material += LoadMaterial;
                    material_panel.material_index = mat_index++;

                    MaterialsPanel.Children.Add(material_panel);
                }
            }
        }

        private void BtnCreate_Click(object sender, RoutedEventArgs e)
        {
            loaded_material = "";
            

            foreach (string material in Materials.Values)
            {
                if (material != String.Empty)
                    loaded_material += material + '|';
                else
                    loaded_material += @"D:\MissingTexture.png" + '|';
            }

            NewDataBaseEntry(loaded_mesh, loaded_material);

            Browser.SelectedIndex = LoadedAssets.Keys.ToList().IndexOf(loaded_mesh);
        }

        private void LoadMaterial(object sender, EventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "png files (*.png)|*.png";
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                SendMessage(pOwner, 0x1202, (IntPtr)((MaterialInput)sender).material_index, Marshal.StringToHGlobalAnsi(openFileDialog.FileName));
                Materials[((MaterialInput)sender).material_index] = openFileDialog.FileName;
                ((MaterialInput)sender).MaterialPath.Text = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void Browser_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (Browser.SelectedItem != null)
            {
                string model = (Browser.SelectedItem as System.Windows.Controls.ListViewItem).ToolTip.ToString().Trim();

                ConnectDatabase(connection, @"SELECT * FROM Models WHERE Mesh = '" + model + "'", out System.Data.SqlClient.SqlConnection sqlConnection, out System.Data.SqlClient.SqlCommand cmd);

                var reader = cmd.ExecuteReader();

                reader.Read();
                string materials = reader.GetString(1);

                SendMessage(pOwner, 0x1203, Marshal.StringToHGlobalAnsi(model), Marshal.StringToHGlobalAnsi(materials));

                loaded_mesh = model;
                loaded_material = materials;
                MeshPath.Text = loaded_mesh;

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
    }
}
