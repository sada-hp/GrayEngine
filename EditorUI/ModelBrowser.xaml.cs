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
        string loaded_mesh = "";
        Dictionary<string, System.Windows.Controls.ListViewItem> LoadedAssets = new Dictionary<string, System.Windows.Controls.ListViewItem>();

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
            LoadData();
        }

        public void ParentRender(IntPtr child)
        {
            child_hwnd = child;
            SetParent(child, panel.Handle);
            UpdateChildPosition();
        }

        private void NewDataBaseEntry(string name)
        {
            if (name != "") //TBD: existence check
            {
                var sqlConnection1 = new System.Data.SqlClient.SqlConnection(@"Data Source=(LocalDB)\MSSQLLocalDB;AttachDbFilename=D:\GrEngine\GrayEngine\EditorUI\Database\Assets_models.mdf;Integrated Security=True");
                var cmd = new System.Data.SqlClient.SqlCommand();
                cmd.CommandType = System.Data.CommandType.Text;
                cmd.CommandText = @"INSERT INTO Models (Mesh) VALUES ('" + name + "')";
                cmd.Connection = sqlConnection1;

                sqlConnection1.Open();
                cmd.ExecuteNonQuery();
                sqlConnection1.Close();
            }

            LoadData();
        }

        private void LoadData()
        {
            ClearBrowser();

            var sqlConnection1 = new System.Data.SqlClient.SqlConnection(@"Data Source=(LocalDB)\MSSQLLocalDB;AttachDbFilename=D:\GrEngine\GrayEngine\EditorUI\Database\Assets_models.mdf;Integrated Security=True"); //TBD
            var cmd = new System.Data.SqlClient.SqlCommand();
            string filter = SearchBar.Text.Replace("Search...", String.Empty);

            cmd.CommandType = System.Data.CommandType.Text;
            cmd.CommandText = "SELECT * FROM Models WHERE Mesh LIKE '%" + filter + "%'";
            cmd.Connection = sqlConnection1;

            sqlConnection1.Open();
            var reader = cmd.ExecuteReader();

            while (reader.Read())
            {
                System.Windows.Controls.ListViewItem item = new System.Windows.Controls.ListViewItem();
                string name = reader.GetString(0);
                item.Content = name + '\n';

                if (!LoadedAssets.ContainsKey(name))
                {
                    LoadedAssets.Add(name, item);
                    Browser.Items.Add(item);
                }
            }

            sqlConnection1.Close();
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
            var sqlConnection1 = new System.Data.SqlClient.SqlConnection(@"Data Source=(LocalDB)\MSSQLLocalDB;AttachDbFilename=D:\GrEngine\GrayEngine\EditorUI\Database\Assets_models.mdf;Integrated Security=True"); //TBD
            var cmd = new System.Data.SqlClient.SqlCommand();
            cmd.CommandType = System.Data.CommandType.Text;
            cmd.CommandText = @"TRUNCATE TABLE Models";
            cmd.Connection = sqlConnection1;

            sqlConnection1.Open();
            cmd.ExecuteNonQuery();
            sqlConnection1.Close();
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
                SendMessage(pOwner, 0x1200, IntPtr.Zero, Marshal.StringToHGlobalAnsi(openFileDialog.FileName));

                loaded_mesh = openFileDialog.FileName;
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
            
            foreach (string material_name in material_names)
            {
                if (material_name != String.Empty)
                {
                    MaterialInput material_panel = new MaterialInput(pOwner);

                    material_panel.NameTextBlock.Text = material_name;
                    System.Windows.Controls.DockPanel.SetDock(material_panel, System.Windows.Controls.Dock.Top);
                    material_panel.material_index = mat_index++;

                    MaterialsPanel.Children.Add(material_panel);
                }
            }
        }

        private void BtnCreate_Click(object sender, RoutedEventArgs e)
        {
            NewDataBaseEntry(loaded_mesh);

            Browser.SelectedIndex = LoadedAssets.Keys.ToList().IndexOf(loaded_mesh);
        }

        private void Browser_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (Browser.SelectedItem != null)
            {
                string model = (Browser.SelectedItem as System.Windows.Controls.ListViewItem).Content.ToString();
                model = model.Trim();

                SendMessage(pOwner, 0x1200, IntPtr.Zero, Marshal.StringToHGlobalAnsi(model));

                loaded_mesh = "";
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
    }
}
