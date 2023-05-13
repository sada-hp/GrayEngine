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
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Runtime.InteropServices;


namespace EditorUI
{
    /// <summary>
    /// Логика взаимодействия для TerrainSettings.xaml
    /// </summary>
    public partial class TerrainSettings : Window
    {
        public static string distr_location = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
        private bool create_mask = false;
        private string save_location = "";
        private int settings_mode;
        public TerrainSettings(int mode)
        {
            InitializeComponent();
            ImgHolder.Source = null;
            ImgHolder.UpdateLayout();
            ImgHolder.ToolTip = "";
            MaskBtn.ToolTip = "";
            BaseBtn.ToolTip = "";
            RedBtn.ToolTip = "";
            GreenBtn.ToolTip = "";
            BlueBtn.ToolTip = "";
            BaseNrm.ToolTip = "";
            RedNrm.ToolTip = "";
            GreenNrm.ToolTip = "";
            BlueNrm.ToolTip = "";
            BaseDis.ToolTip = "";
            RedDis.ToolTip = "";
            GreenDis.ToolTip = "";
            BlueDis.ToolTip = "";
            settings_mode = mode;
            if (settings_mode == 1)
            {
                Tab.Items.RemoveAt(0);

                string mask = Marshal.PtrToStringAnsi(UIBridge.GetTerrainMask());
                if (mask != "")
                {
                    ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + mask), Rotation.Rotate0));
                    brush.Stretch = Stretch.Uniform;
                    MaskBtn.Background = brush;
                    MaskBtn.ToolTip = distr_location + '\\' + mask;
                }

                string color = Marshal.PtrToStringAnsi(UIBridge.GetTerrainColor());
                if (color != "")
                {
                    var arr = color.Split('|');

                    if (arr[0] != "" && arr[0] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[0]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        BaseBtn.Background = brush;
                        BaseBtn.ToolTip = distr_location + '\\' + arr[0];
                    }

                    if (arr[1] != "" && arr[1] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[1]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        RedBtn.Background = brush;
                        RedBtn.ToolTip = distr_location + '\\' + arr[1];
                    }

                    if (arr[2] != "" && arr[2] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[2]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        GreenBtn.Background = brush;
                        GreenBtn.ToolTip = distr_location + '\\' + arr[2];
                    }

                    if (arr[3] != "" && arr[3] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[3]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        BlueBtn.Background = brush;
                        BlueBtn.ToolTip = distr_location + '\\' + arr[3];
                    }
                }

                string normal = Marshal.PtrToStringAnsi(UIBridge.GetTerrainNormal());
                if (normal != "")
                {
                    var arr = normal.Split('|');

                    if (arr[0] != "" && arr[0] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[0]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        BaseNrm.Background = brush;
                        BaseNrm.ToolTip = distr_location + '\\' + arr[0];
                    }

                    if (arr[1] != "" && arr[1] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[1]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        RedNrm.Background = brush;
                        RedNrm.ToolTip = distr_location + '\\' + arr[1];
                    }

                    if (arr[2] != "" && arr[2] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[2]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        GreenNrm.Background = brush;
                        GreenNrm.ToolTip = distr_location + '\\' + arr[2];
                    }

                    if (arr[3] != "" && arr[3] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[3]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        BlueNrm.Background = brush;
                        BlueNrm.ToolTip = distr_location + '\\' + arr[3];
                    }
                }

                string displacement = Marshal.PtrToStringAnsi(UIBridge.GetTerrainDisplacement());
                if (displacement != "")
                {
                    var arr = displacement.Split('|');

                    if (arr[0] != "" && arr[0] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[0]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        BaseDis.Background = brush;
                        BaseDis.ToolTip = distr_location + '\\' + arr[0];
                    }

                    if (arr[1] != "" && arr[1] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[1]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        RedDis.Background = brush;
                        RedDis.ToolTip = distr_location + '\\' + arr[1];
                    }

                    if (arr[2] != "" && arr[2] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[2]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        GreenDis.Background = brush;
                        GreenDis.ToolTip = distr_location + '\\' + arr[2];
                    }

                    if (arr[3] != "" && arr[3] != "empty_texture")
                    {
                        ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(distr_location + '\\' + arr[3]), Rotation.Rotate0));
                        brush.Stretch = Stretch.UniformToFill;
                        BlueDis.Background = brush;
                        BlueDis.ToolTip = distr_location + '\\' + arr[3];
                    }
                }
            }

            EmptySettings.Width = new GridLength(0);
            EmptyButton.Width = new GridLength(45, GridUnitType.Star);
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (create_mask && (MaskWidth.Text == "" || MaskHeight.Text == "" || save_location == ""))
            {
                MessageBox.Show("Enter valid values in blend mask section!", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                Tab.SelectedItem = TabMaps;
                return;
            }
            else if (create_mask)
            {
                UIBridge.WriteImage(Marshal.StringToHGlobalAnsi(save_location), int.Parse(MaskWidth.Text), int.Parse(MaskHeight.Text));
            }

            string blend_mask = MaskBtn.ToolTip.ToString();
            string base_layer = BaseBtn.ToolTip.ToString();
            string red_ch = RedBtn.ToolTip.ToString();
            string green_ch = GreenBtn.ToolTip.ToString();
            string blue_ch = BlueBtn.ToolTip.ToString();
            string base_nrm = BaseNrm.ToolTip.ToString();
            string red_nrm = RedNrm.ToolTip.ToString();
            string green_nrm = GreenNrm.ToolTip.ToString();
            string blue_nrm = BlueNrm.ToolTip.ToString();
            string base_dis = BaseDis.ToolTip.ToString();
            string red_dis = RedDis.ToolTip.ToString();
            string green_dis = GreenDis.ToolTip.ToString();
            string blue_dis = BlueDis.ToolTip.ToString();

            if (settings_mode == 1)
            {

                UIBridge.UpdateTerrain(Marshal.StringToHGlobalAnsi(blend_mask), Marshal.StringToHGlobalAnsi(base_layer), Marshal.StringToHGlobalAnsi(red_ch), Marshal.StringToHGlobalAnsi(green_ch), Marshal.StringToHGlobalAnsi(blue_ch),
                    Marshal.StringToHGlobalAnsi(base_nrm), Marshal.StringToHGlobalAnsi(red_nrm), Marshal.StringToHGlobalAnsi(green_nrm), Marshal.StringToHGlobalAnsi(blue_nrm),
                    Marshal.StringToHGlobalAnsi(base_dis), Marshal.StringToHGlobalAnsi(red_dis), Marshal.StringToHGlobalAnsi(green_dis), Marshal.StringToHGlobalAnsi(blue_dis));
            }
            else
            {
                string height_map = ImgHolder.ToolTip.ToString();
                int resolution;
                int width;
                int height;
                int depth;

                bool res = int.TryParse(ResolutionTB.Text, out resolution);
                resolution = res && resolution >= 2 ? resolution : 2;

                res = int.TryParse(WidthTB.Text, out width);
                width = res && width >= 1 ? width : 1;

                res = int.TryParse(HeightTB.Text, out height);
                height = res && height >= 1 ? height : 1;

                res = int.TryParse(DepthTB.Text, out depth);
                depth = res && depth >= 1 ? depth : 1;

                UIBridge.GenerateTerrain(resolution, width, height, depth, Marshal.StringToHGlobalAnsi(height_map),
                    Marshal.StringToHGlobalAnsi(blend_mask), Marshal.StringToHGlobalAnsi(base_layer), Marshal.StringToHGlobalAnsi(red_ch), Marshal.StringToHGlobalAnsi(green_ch), Marshal.StringToHGlobalAnsi(blue_ch),
                     Marshal.StringToHGlobalAnsi(base_nrm), Marshal.StringToHGlobalAnsi(red_nrm), Marshal.StringToHGlobalAnsi(green_nrm), Marshal.StringToHGlobalAnsi(blue_nrm),
                     Marshal.StringToHGlobalAnsi(base_dis), Marshal.StringToHGlobalAnsi(red_dis), Marshal.StringToHGlobalAnsi(green_dis), Marshal.StringToHGlobalAnsi(blue_dis));

                ImgHolder.Source = null;
                ImgHolder.UpdateLayout();
            }

            MaskBtn.Background = null;
            BaseBtn.Background = null;
            RedBtn.Background = null;
            GreenBtn.Background = null;
            BlueBtn.Background = null;
            MaskBtn.Background = null;
            BaseNrm.Background = null;
            RedNrm.Background = null;
            GreenNrm.Background = null;
            BlueNrm.Background = null;
            BaseDis.Background = null;
            RedDis.Background = null;
            GreenDis.Background = null;
            BlueDis.Background = null;
            MaskBtn.UpdateLayout();
            BaseBtn.UpdateLayout();
            RedBtn.UpdateLayout();
            GreenBtn.UpdateLayout();
            BlueBtn.UpdateLayout();
            BaseNrm.UpdateLayout();
            RedNrm.UpdateLayout();
            GreenNrm.UpdateLayout();
            BlueNrm.UpdateLayout();
            BaseDis.UpdateLayout();
            RedDis.UpdateLayout();
            GreenDis.UpdateLayout();
            BlueDis.UpdateLayout();

            Close();
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, true);
        }

        private void Cancel_Click(object sender, RoutedEventArgs e)
        {
            Close();
            GC.Collect();
        }

        private void TextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            System.Text.RegularExpressions.Regex reg = new System.Text.RegularExpressions.Regex("[0-9]");
            if (!reg.IsMatch(e.Text))
            {
                e.Handled = true;
            }
        }
        private ImageSource BitmapFromUri(Uri src, Rotation rot)
        {
            var btm = new BitmapImage();
            btm.BeginInit();
            btm.UriSource = src;
            btm.CacheOption = BitmapCacheOption.None;
            btm.Rotation = rot;
            btm.EndInit();
            btm.Freeze();

            return btm;
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImgHolder.Source = BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0);
                ImgHolder.ToolTip = openFileDialog.FileName;
                ImgHolder.UpdateLayout();
                LoadBtn.Background = null;
                LoadBtn.Content = "";
            }

            GC.Collect();
        }

        private void MaskBtn_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.Uniform;
                MaskBtn.Background = brush;
                MaskBtn.ToolTip = openFileDialog.FileName;
                create_mask = false;
                EmptySettings.Width = new GridLength(0);
                EmptyButton.Width = new GridLength(45, GridUnitType.Star);
            }

            GC.Collect();
        }

        private void CreateMask_Click(object sender, RoutedEventArgs e)
        {
            EmptyButton.Width = new GridLength(0);
            EmptySettings.Width = new GridLength(45, GridUnitType.Star);
            create_mask = true;
            MaskBtn.Background = new SolidColorBrush(Color.FromRgb(0, 0, 0));
            MaskBtn.ToolTip = "";
        }

        private void MaskSave_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.SaveFileDialog openFileDialog = new System.Windows.Forms.SaveFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                MaskBtn.ToolTip = openFileDialog.FileName;
                save_location = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void BaseBtn_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                BaseBtn.Background = brush;
                BaseBtn.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void RedBtn_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                RedBtn.Background = brush;
                RedBtn.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void GreenBtn_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                GreenBtn.Background = brush;
                GreenBtn.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void BlueBtn_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                BlueBtn.Background = brush;
                BlueBtn.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void BaseNrm_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                BaseNrm.Background = brush;
                BaseNrm.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void RedNrm_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                RedNrm.Background = brush;
                RedNrm.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void GreenNrm_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                GreenNrm.Background = brush;
                GreenNrm.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void BlueNrm_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                BlueNrm.Background = brush;
                BlueNrm.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void BaseDis_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                BaseDis.Background = brush;
                BaseDis.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void RedDis_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                RedDis.Background = brush;
                RedDis.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void GreenDis_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                GreenDis.Background = brush;
                GreenDis.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }

        private void BlueDis_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                ImageBrush brush = new ImageBrush(BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0));
                brush.Stretch = Stretch.UniformToFill;
                BlueDis.Background = brush;
                BlueDis.ToolTip = openFileDialog.FileName;
            }

            GC.Collect();
        }
    }
}
