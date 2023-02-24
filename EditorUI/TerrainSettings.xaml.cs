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
        public TerrainSettings()
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
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
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

            string height_map = ImgHolder.ToolTip.ToString();
            string blend_mask = MaskBtn.ToolTip.ToString();
            string base_layer = BaseBtn.ToolTip.ToString();
            string red_ch = RedBtn.ToolTip.ToString();
            string green_ch = GreenBtn.ToolTip.ToString();
            string blue_ch = BlueBtn.ToolTip.ToString();

            UIBridge.GenerateTerrain(resolution, width, height, depth, Marshal.StringToHGlobalAnsi(height_map), Marshal.StringToHGlobalAnsi(blend_mask), Marshal.StringToHGlobalAnsi(base_layer), Marshal.StringToHGlobalAnsi(red_ch), Marshal.StringToHGlobalAnsi(green_ch), Marshal.StringToHGlobalAnsi(blue_ch));
            Close();
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, true);
        }

        private void Cancel_Click(object sender, RoutedEventArgs e)
        {
            Close();
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
            }
        }
    }
}
